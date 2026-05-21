# Deep review — `src/venda.cpp`

> Resultado de revisão manual após o fix de `Venda::verifyFields` (commit `10cadbdd`), motivado pela divergência de valores da NF-e da venda **ALPH-260435**. Este documento registra a dívida técnica conhecida do arquivo (1920 linhas) para priorização futura. Nenhum fix deste review foi implementado ainda — todos os itens permanecem em aberto.

Cada item indica: linhas afetadas → descrição → impacto → recomendação.

---

## A. CRÍTICOS (perda de dados, dados inconsistentes, regressões prováveis)

### A1. `Venda::generateId()` — race condition em PK (l.1348-1372)

```cpp
query.prepare("SELECT MAX(idVenda) AS idVenda FROM venda WHERE idVenda LIKE :id");
// ...
last = query.value("idVenda").toString().mid(7, 4).toInt();
id += QString::number(last + 1).rightJustified(4, '0');
```

Sem `SELECT ... FOR UPDATE` nem mecanismo de unicidade no banco. Dois usuários cadastrando venda na mesma loja, mesmo ano, em paralelo, geram o mesmo `idVenda`. O segundo `INSERT` falha por PK e o rollback (l.1134) recoloca os itens no backup, mas o usuário vê um erro genérico. Pior cenário: se a unique-constraint estiver afrouxada (e a tabela `venda` permite por exemplo idVenda como char(11) sem UNIQUE), duas vendas com mesmo ID convivem e ficam corrompidas.

**Impacto**: corrupção de PK ou erro inexplicável para usuário em carga real.
**Fix**: gerar ID dentro da transação com `SELECT ... FOR UPDATE` na linha de controle (ou tabela de sequência por loja+ano), ou capturar erro de duplicate-key e reentrar `generateId()` até N tentativas.

### A2. Regressão do `verifyFields` em vendas legadas inconsistentes (l.1421 → 540)

`on_pushButtonFinanceiroSalvar_clicked` chama `verifyFields()`, que agora roda `verificarTotais()`. Vendas antigas com `subTotalLiq − descontoReais + frete ≠ total` (caso ALPH-260435 e possivelmente outras) vão estourar `"Erro nos valores! Entre em contato com o suporte!"` ao salvar **apenas o financeiro**, mesmo que a operação financeira não toque nesses campos.

**Impacto**: bloqueia operações financeiras (mudar status, comissão, taxa) de qualquer venda legada com defeito histórico.
**Fix**: tornar `verificarTotais()` no-op em modo `financeiro` (l.1392), OU rodar `corrigirValores()` + recálculo de `total` via handler ANTES de validar quando vier de modo financeiro, OU pular a invariante do frete (`freteErrado`) em modo financeiro, deixando as outras 3 ativas.

### A3. Divisão por zero em handlers de total/desconto (l.886-913, 971-998, 1000-1027)

```cpp
const double descontoPorc = descontoReais / subTotalLiq;   // l.893, l.1005
```

Se `subTotalLiq = 0` (venda só com itens cancelados/zerados), divide por zero. NaN propaga para `descGlobal` e `total` de cada item (l.896, 899, 977-981, 1008-1011), corrompendo a venda inteira.

**Impacto**: vendas em edição com subtotal zero geram NaN salvo no banco.
**Fix**: guard `if (qFuzzyIsNull(subTotalLiq)) { return; }` no início de cada handler antes da divisão.

### A4. `prepararVenda` (l.313) carrega tudo com signals desconectados

`setConnections()` só é chamado em l.427, após todos os `setValue` (l.342-360). Logo, nenhum handler de reconciliação dispara — os widgets refletem literalmente os campos do orçamento, mesmo que internamente inconsistentes. **Root cause documentado do caso ALPH-260435.**

**Impacto**: venda herda inconsistência do orçamento sem detecção. Mitigado parcialmente por `verificarTotais()` no salvamento, mas só após o usuário tentar gravar.
**Fix**: depois de carregar todos os widgets, chamar `verificarTotais()` explicitamente em `prepararVenda` (ainda dentro do `unsetConnections`), e se inconsistente, marcar UI em vermelho ou avisar antes do usuário começar a editar.

### A5. `viewRegister` (l.690-774) e clipping silencioso no carregamento

Mesmo problema do A4 ao abrir uma venda existente. Combinado com o `setMinimum` que o `calcularFrete()` chama em l.348, o widget do frete pode mostrar um valor DIFERENTE do banco no instante em que a tela abre (banco tem R$ 240, widget mostra R$ 288 porque `minimoGerente = 288`). O usuário então edita outros campos, o handler do frete não dispara (ele não mexeu no frete), e a próxima gravação salva R$ 288 no `frete` mesmo que o banco tivesse R$ 240. **Ampliação silenciosa do bug ALPH-260435.**

**Impacto**: divergência entre banco e tela após carregamento.
**Fix**: nunca alterar o valor exibido durante a carga; se o frete carregado for menor que `minimoGerente`, exibir aviso visível em vez de clipar.

---

## B. IMPORTANTES (UX, autorização, auditoria)

### B1. `canChangeFrete` e "Frete Manual" resetados ao trocar endereço (l.1511-1514)

```cpp
void Venda::on_itemBoxEndereco_idChanged() {
  if (User::isGerente()) { minimoGerente = 0.; }
  canChangeFrete = false;                           // ← perde autorização
  ui->checkBoxFreteManual->setChecked(false);       // ← desmarca silenciosamente
  ui->checkBoxFreteManual->setEnabled(true);
  // ...
  calcularFrete(true);                              // ← reescreve frete
}
```

Gerente entra na tela, marca "Frete Manual", digita valor, depois muda endereço (por engano ou propositalmente) → frete é reescrito sem aviso, "Frete Manual" some, gerente precisa reautorizar com administrativo.

**Impacto**: trabalho perdido, frustração; em carga, gerente pode salvar o frete recalculado achando que está com o manual.
**Fix**: se `checkBoxFreteManual->isChecked()`, preservar `canChangeFrete` e o valor manual; apenas pedir confirmação ao usuário se ele quer recalcular pelo novo endereço.

### B2. `setMinimum` clipa silenciosamente (l.946, 1616)

`QDoubleSpinBox::setMinimum` força o valor para o piso sem feedback se o usuário digita abaixo. Combina com B1 e A5.

**Impacto**: usuário digita R$ 200, vê R$ 288, não entende, repete, perde a fé no sistema.
**Fix**: usar `setMinimum(0)` e validar manualmente com aviso visível (`enqueueWarning`) quando o usuário tentar salvar abaixo do permitido.

### B3. Permissões só via UI hide (l.756, 758, 1389)

```cpp
if (not User::isAdministrativo() and not User::isGerente()) { ui->pushButtonCancelamento->hide(); }
if (not User::isAdministrativo()) { ui->pushButtonCorrigirFluxo->hide(); }
```

A função `cancelamento()` (l.1159) e o fluxo de correção não verificam permissão. Hoje só são chamados por handlers que estão escondidos — mas qualquer atalho de teclado, evento programático ou refatoração futura libera o caminho.

**Impacto**: defesa em profundidade ausente; depende de UI estar consistente.
**Fix**: chamar `if (not User::isAdministrativo() and not User::isGerente()) { throw RuntimeError("Sem permissão!"); }` no início de `cancelamento()`. Idem em `on_pushButtonCorrigirFluxo_clicked`.

### B4. `Venda::cancelamento()` sem auditoria (l.1159, TODO l.1160)

Sem registro de quem cancelou nem quando. Apenas `UPDATE venda SET status = 'CANCELADO'`. Para um cancelamento que zera estoque, fluxo de caixa, comissões e crédito do cliente, isso é grave.

**Impacto**: impossível investigar cancelamentos em disputa.
**Fix**: adicionar colunas `canceladoPor`, `canceladoEm`, `motivoCancelamento` em `venda`; salvar na transação de cancelamento. Combina com TODO l.1253 ("perguntar motivo").

### B5. `montarFluxoCaixa` chamado múltiplas vezes por operação (TODO l.863)

Vários handlers chamam `montarFluxoCaixa()` (l.906, 962, 991, 1020) + `widgetPgts` o emite via signal (l.187). Sem debounce, sequências de `setValue` que disparam múltiplos handlers chamam montar várias vezes. Cada chamada faz `modelFluxoCaixa.revertAll()` + reinsere — caro e potencialmente duplicador se um signal escapa do `unsetConnections`.

**Impacto**: performance ruim na edição; risco de duplicar/perder linhas em race.
**Fix**: debounce com `QTimer` de 50ms; chamar `montarFluxoCaixa` no expirar.

### B6. Tolerância de R$ 0,10 em pagamentos sem feedback (l.540)

```cpp
if (abs(ui->widgetPgts->getTotalPag() - ui->doubleSpinBoxTotal->value()) > 0.1)
  throw RuntimeError("Total dos pagamentos difere do total do pedido!");
```

R$ 0,10 é tolerável, mas o TODO de l.534 ("pintar totalPag de vermelho enquanto o total for diferente") diz que o feedback visual ainda não existe. Usuário só vê o erro ao tentar salvar.

**Impacto**: retrabalho do usuário; pode mascarar erro de rateio em parcelas com muitos centavos sobrando.
**Fix**: ligar `widgetPgts->totalPag` ao `doubleSpinBoxTotal` em tempo real, pintar de vermelho enquanto divergir > 0.10.

### B7. `status` vs `statusFinanceiro` desacoplados (TODO l.2018)

A própria nota do código diz que talvez `status` deveria ficar `LIBERADO` quando todos os pagamentos vão para `RECEBIDO`. Hoje cabe ao financeiro lembrar de mudar manualmente. Caminho fácil para venda paga em produção continuar `EM COMPRA`/`EM FATURAMENTO` indefinidamente.

**Impacto**: state machine inconsistente; relatórios de pendências erram.
**Fix**: criar trigger (no banco) ou método em `Sql::` que recalcule `status` quando `statusFinanceiro` muda para `LIBERADO`. Documentar a regra e fechar o TODO.

---

## C. CÓDIGO / MANUTENÇÃO

### C1. N+1 queries em `calcularFrete` (l.1565-1589) e `calcularPesoTotal` (l.1779-1797)

Loops com `SELECT kgcx FROM produto WHERE idProduto = X` por item. Para venda com 50 itens: ~100 round-trips no MySQL síncrono.
**Fix**: uma query agregada `WHERE idProduto IN (?, ?, ...)`.

### C2. SQL concatenado em pontos sem aspas (l.698, 751, 824, 918-919, 1128, 1132, 1284, 1570, 1580, 1659, 1769, 1785, 1998)

Maior risco em `trocarEnderecoEntrega` (l.1998): `WHERE idVenda = '" + primaryId + "'"`. Hoje seguro (idVenda gerado internamente), mas não há defesa caso `primaryId` venha alterado de fora.
**Fix**: trocar para `prepare` + `bindValue` em todos os lugares.

### C3. Implicit double → int em peso (TODO l.1795)

`ui->spinBoxPesoTotal->setValue(total)` onde `total` é double — `spinBoxPesoTotal` é `QSpinBox` (int). Perde fração.
**Fix**: trocar para `QDoubleSpinBox` ou exibir em kg arredondado claramente.

### C4. TODOs pendentes de impacto real

- **l.534**: feedback visual de tolerância de pagamento (mencionado em B6).
- **l.718**: "quando estiver tudo pago bloquear correção de fluxo".
- **l.1167**: orçamento sendo reativado mesmo fora da validade.
- **l.1217**: bug conhecido em `ALPH-211411` sem fix.
- **l.1253-1254**: motivo de cancelamento + cancelamento de agendamento.
- **l.1296**: 2 formas de cancelar pagamento sem unificação.
- **l.2017**: corrigir fluxo mostra botões só de representação em pedidos comuns.
- **l.2020**: sem limite máximo de desconto por classe de funcionário (vendedor pode dar 90% silenciosamente).

---

## Prioridade recomendada de fix

| # | Item | Severidade | Risco se não fizer | Esforço |
|---|---|---|---|---|
| 1 | **A2 — regressão `verifyFields` em vendas legadas** | Crítico | Bloqueia financeiro de vendas antigas (alto impacto operacional imediato) | Baixo |
| 2 | **A3 — division by zero nos handlers** | Crítico | NaN salvo no banco (corrupção silenciosa) | Baixo |
| 3 | **B1 + B2 — frete manual do GERENTE LOJA** | Importante | Frustração + dado errado salvo | Médio |
| 4 | **A4 + A5 — chamar `verificarTotais` ao final de `prepararVenda`/`viewRegister`** | Crítico | Detecta inconsistências na carga, antes da edição | Baixo |
| 5 | **A1 — race condition em `generateId`** | Crítico | Conflito de PK em carga simultânea | Médio |
| 6 | **B4 — auditoria de cancelamento** | Importante | Impossível investigar disputas | Baixo |
| 7 | **B3 — permissão server-side em cancelamento/corrigirFluxo** | Importante | Defesa em profundidade | Baixo |
| 8 | **B7 — vincular `status` ao `statusFinanceiro`** | Importante | Relatórios incorretos | Médio |
| 9 | **C1-C3 — N+1, SQL concatenado, double→int** | Manutenção | Performance + manutenibilidade | Médio |

---

## Verificação (para quando algum item for implementado)

1. `qmake Loja.pro && make` deve compilar limpo.
2. Para A2, query retrospectiva:
   ```sql
   SELECT idVenda, subTotalLiq, descontoReais, frete, total,
          ROUND(subTotalLiq - descontoReais + frete - total, 2) AS diff
     FROM venda
    WHERE ABS(subTotalLiq - descontoReais + frete - total) > 0.10
    ORDER BY ABS(subTotalLiq - descontoReais + frete - total) DESC;
   ```
   Confirmar que vendas legadas inconsistentes salvam financeiro sem estourar.
3. Para A1, simular cadastro concorrente com 2 instâncias do app abertas; ambas devem terminar com idVenda distintos.
4. Para A3, criar venda com todos os itens com `quant = 0` (subTotalLiq = 0); mover o spin de desconto/total; confirmar que nenhum NaN entra nos itens.
5. Para B1/B2, percorrer o fluxo: GERENTE LOJA abre venda, marca "Frete Manual", digita valor, muda endereço → confirmar que o frete manual e o checkbox permanecem.
