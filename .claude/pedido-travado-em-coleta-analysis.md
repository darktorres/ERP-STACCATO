# Relatório: pedidos de compra travados em status pré-recebimento

> Investigação de 2026-07 (código + dados do banco **live**, com teste preditivo e validação
> adversarial). Diagnóstico do bug relatado como "produtos em recebimento que viram pedidos" /
> "travam no status em recebimento".

## Visão geral

Existem **dois problemas distintos**. O dominante e ativo é uma corrida (Issue 1); o outro é
real mas raro/histórico (Issue 2).

| | Issue 1 — Corrida consumo↔recebimento | Issue 2 — Vínculo zerado sem reset |
|---|---|---|
| Sintoma | Compra travada em `EM COLETA`, venda já entregue | Compra órfã congelada em `EM RECEBIMENTO` |
| Volume live | ~1.269 linhas presas; ~891 com venda já terminal | 42 linhas (16 totalmente órfãs) |
| Está ativo? | **Sim** — 10 a 70 novas/mês até jul/2026 | Não — nada desde out/2021 |
| Origem | `Estoque::dividirCompra` (fatiamento no consumo de estoque) | 3 pontos que zeram `idVenda` sem tratar status |

---

## Issue 1 — Corrida "consumo antes do recebimento" (dominante)

### O que acontece, em palavras

Quando uma venda consome um lote de estoque que **já chegou fisicamente mas cuja O.C. ainda não
foi "Marcada Recebida"**, o sistema fatia a linha de compra e cria um **filho** que herda o
status pré-recebimento do pai e **não recebe vínculo `estoque_has_compra` próprio**. Depois, ao
marcar o recebimento, só o pai avança — o filho fica preso em `EM COLETA` para sempre, mesmo
depois da venda ser entregue.

### Exemplo real: família da O.C. do pedido 250544 (PORTINARI 6062813A)

Um único lote de estoque (117565, 205,92 un., NF-e 121479) foi fatiado entre várias vendas.
O **momento do nascimento** de cada filho determinou seu destino:

| idPedido2 | idRelacionado | status | venda | quant | nasceu em | receb. próprio? |
|---|---|---|---|---|---|---|
| 250544 (**pai**) | — | `ESTOQUE` | (sem venda) | 28,80 | 2026-06-01 | sim |
| 251507 | 250544 | **`EM COLETA`** (preso) | GABR-260323 | 66,24 | 2026-06-19 | não |
| 251508 | 250544 | **`EM COLETA`** (preso) | GABR-260293 | 59,04 | 2026-06-19 | não |
| 251962 | 250544 | `ENTREGUE` (ok) | BRSL-260822 | 28,80 | 2026-06-25 | não |
| 252291 | 250544 | `ESTOQUE` (ok) | ALPH-260540 | 11,52 | 2026-06-30 | não |
| 252315 | 250544 | `ENTREGA AGEND.` (ok) | GABR-260203 | 2,88 | 2026-06-30 | não |

**Nenhum** filho tem recebimento próprio — mas só os dois nascidos em **19/06** (antes do
recebimento do pai) ficaram presos. Os nascidos em 25/06 e 30/06 fluíram normalmente.

### A linha do tempo que explica

| Data | Evento | Efeito na compra |
|---|---|---|
| **18/06** | NF-e 121479 importada | Cria `estoque` 117565 + `estoque_has_compra`→**250544**; O.C. fica em `EM COLETA` |
| **19/06** | Venda GABR-260293 consome o lote (**antes** do "Marcar Recebido") | `dividirCompra` fatia → filho **251508 nasce `EM COLETA`, sem `estoque_has_compra`** |
| **22/06** | "Marcar Recebido" do lote 117565 | `UPDATE` filtra por `estoque_has_compra` → **só 250544 vira `ESTOQUE`**; 251508 continua `EM COLETA` |
| depois | Venda GABR-260293 entregue | Venda = `ENTREGUE`, mas compra 251508 continua `EM COLETA` |
| 25/06+ | Irmãos nascem **após** o recebimento | Nascem `ESTOQUE` → seguem até `ENTREGUE` sem problema |

### Por que fica preso: como cada transição de status é filtrada

O filho **tem** `idVendaProduto2` (o vínculo da venda), mas está preso *antes* de `ESTOQUE`,
exatamente onde as transições exigem o `estoque_has_compra` que ele não tem:

| Transição (na tabela de compra) | Arquivo:linha | Filtra por | Alcança o filho órfão? |
|---|---|---|---|
| `EM COLETA` → `EM RECEBIMENTO` | widgetlogisticacoleta.cpp:124 | `estoque_has_compra` | **não** |
| `EM COLETA` → `ESTOQUE` | widgetlogisticaagendarcoleta.cpp:265 | `estoque_has_compra` | **não** |
| `EM RECEBIMENTO` → `ESTOQUE` | widgetlogisticarecebimento.cpp:100 | `estoque_has_compra` | **não** |
| `ESTOQUE` → `ENTREGA AGEND.` | widgetlogisticaagendarentrega.cpp:489 | `idVendaProduto2` | sim |
| → `EM ENTREGA` | cadastrarnfe.cpp:288 | `idVendaProduto2` | sim |
| → `ENTREGUE` | widgetlogisticaentregas.cpp:301 | `idVendaProduto2` | sim |

Os nascidos já em `ESTOQUE` fluem (usam a chave `idVendaProduto2`); os presos em `EM COLETA`
não passam do portão do recebimento (chave `estoque_has_compra`).

### Prova em escala (teste preditivo)

Se a teoria estiver certa, filhos **presos** devem ter nascido **antes** do recebimento do pai,
e os **saudáveis**, depois. Rodado sobre ~32 mil filhos de pais já recebidos:

| status do filho | total | nasceu ANTES do recebimento | nasceu DEPOIS |
|---|---|---|---|
| `ENTREGUE` (ok) | 24.939 | 372 | **24.567** |
| `ESTOQUE` (ok) | 3.649 | 15 | **3.634** |
| `EM ENTREGA` (ok) | 1.475 | 1 | **1.474** |
| `ENTREGA AGEND.` (ok) | 966 | 9 | **957** |
| **`EM COLETA` (preso)** | **1.049** | **860** | 169 |

Distribuição **exatamente oposta**: saudáveis nascem depois; presos nascem antes (82%).

### A venda seguiu, a compra não (assimetria de lockstep)

Para as 1.269 compras presas em `EM COLETA`, o status da **venda** ligada:

| status da VENDA | nº de compras presas |
|---|---|
| `ENTREGUE` | 831 |
| `ESTOQUE` | 51 |
| `QUEBRADO` | 22 |
| `EM COLETA` (ainda em lockstep) | 11 |
| outros | 12 |

Em 99% dos casos a venda já andou (a maioria totalmente entregue) enquanto a compra ficou para
trás.

### Está gerando novos casos todo mês

| Mês | novos filhos presos em `EM COLETA` |
|---|---|
| 2026-07 (parcial) | 4 |
| 2026-06 | 37 |
| 2026-05 | 20 |
| 2026-04 | 25 |
| 2026-03 | 71 |
| 2026-02 | 12 |
| 2025-11 | 29 |
| 2025-10 | 50 |

### Backlog reconciliável hoje (venda já terminal, compra sem recebimento próprio)

| status da compra | linhas |
|---|---|
| `EM COLETA` | 868 |
| `EM RECEBIMENTO` | 23 |
| **Total** | **891** |

### Onde está o código

- `Estoque::criarConsumo` (`estoque.cpp:186-278`) → chama `Estoque::dividirCompra`
  (`estoque.cpp:280-356`): o loop de cópia (linhas 331-343) copia **todas** as colunas do pai,
  inclusive `status`, e **nunca** cria `estoque_has_compra` para o filho.
- Gêmeo idêntico em `inputdialogconfirmacao.cpp:945-962`.
- Chamadores: `ProdutosPendentes::consumirEstoque` (botão "Consumir Estoque") e
  `Venda::criarConsumos` (`venda.cpp:1176`, **automático no save de toda venda** com item de
  estoque).

### Ressalva honesta

"Permanente" vale no fluxo normal/forward. Cancelar a **entrega** da venda
(`widgetlogisticaentregas.cpp:423` / `widgetlogisticaentregues.cpp:172`, filtrado por
`idVendaProduto2`, admite `EM COLETA`) empurraria o filho para `ESTOQUE` incidentalmente — mas
é reversão manual de entrega, nunca progressão de rotina. Nenhuma rotina de
reconciliação/cron/trigger corrige (triggers só copiam status pf2→pf1; `widgetconsistencia.cpp`
é só leitura).

---

## Issue 2 — Vínculo zerado sem reset de status (secundário, raro)

### O que acontece

Três funções zeram `idVenda`/`idVendaProduto2` da compra **sem tocar no `status`**. Se a compra
estava em `EM RECEBIMENTO`/`ESTOQUE`, ela fica órfã e congelada nesse status.

| Função | Arquivo:linha | Guarda de status hoje | Problema |
|---|---|---|---|
| `Estoque::desfazerConsumo` | estoque.cpp:373 | chamador (`widgetcompraconsumos.cpp:91-97`) **não** bloqueia `EM RECEBIMENTO`/`ESTOQUE` | zera vínculo, não reseta status |
| `Devolucao::desvincularCompra` | devolucao.cpp:451 | **nenhuma** | idem |
| `Venda::cancelamento` | venda.cpp:1233 | precheck (venda.cpp:1341) **pula** itens já em estoque | idem |

### Footprint no live

42 linhas em `EM RECEBIMENTO` (16 totalmente órfãs, `idVenda` NULL, sem `estoque_has_compra`),
**todas de 2021 ou antes** — o mecanismo é real mas praticamente não dispara hoje.

### Contraste com a referência correta

`WidgetNfeEntrada::inutilizar` (`widgetnfeentrada.cpp:296-299`) faz o certo: ao desvincular,
também reseta o `status` para `EM FATURAMENTO` (estágio revisável). Os 3 pontos acima deveriam
seguir esse padrão.

---

## Resumo dos fixes propostos

| # | Ação | Onde | Risco |
|---|---|---|---|
| A | Filho fatiado nasce com o status do estoque consumido (não do pai) | `estoque.cpp:280-356` + `inputdialogconfirmacao.cpp:945` | **Alto** (caminho central) — exige reprodução dirigida |
| B | Reconciliar ~891 presos (dry-run guiado) | script novo em `db/` | Médio (dados prod) |
| C | Query de monitoramento contínuo | `db/diagnostico_pedido_travado_recebimento.sql` | Nenhum (read-only) |
| D | Resetar status nos 3 pontos do Issue 2 | estoque.cpp:373, devolucao.cpp:451, venda.cpp:1233 | Baixo |

### Como validar o fix A (reprodução dirigida)

Em ambiente de teste: importar NF-e de um produto (cria estoque, O.C. fica `EM COLETA`),
consumir esse estoque numa venda **antes** de "Marcar Recebido", depois marcar recebido —
confirmar que hoje o filho fica preso em `EM COLETA`; aplicar o fix e confirmar que passa a
nascer em `ESTOQUE` e flui até `ENTREGUE`. Rodar a query de monitoramento (C) no live
antes/depois.
