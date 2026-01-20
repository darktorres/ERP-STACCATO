# Relatório: Preenchimento de Impostos nas NFes

**Data**: 19/01/2026
**Destinatário**: Contabilidade
**Assunto**: Como o sistema preenche os impostos nas Notas Fiscais Eletrônicas

---

## 1. Origem dos Dados

Todos os dados de impostos vêm da **Tabela NCM** (Nomenclatura Comum do Mercosul).

Para cada produto, o sistema consulta o NCM e extrai:
- `st` - Tem Substituição Tributária? (SIM/NÃO)
- `aliq` - Taxa ICMS (ex: 7.00%)
- `cClassTribIBS` - Classificação IBS (ex: 300001)
- `cClassTribCBS` - Classificação CBS (ex: 300001)
- `sujeitoIS` - Sujeito a Imposto Seletivo? (SIM/NÃO)
- `pIS` - Taxa IS (ex: 10.50%)
- `cClassTribIS` - Classificação IS (ex: 620001)

**Validação**: Se algum campo obrigatório estiver vazio, a NFe é rejeitada com erro.

---

## 2. Preenchimento dos Impostos por Tipo de NFe

### Tipo: ENTRADA (Devolução de Cliente)

**De onde vem**:
- CFOP: Determinado pelo `st` da NCM
  - Se `st = SIM` → CFOP 1411/2411
  - Se `st = NÃO` → CFOP 1202/2202
- CST ICMS:
  - Se `st = SIM` → 60
  - Se `st = NÃO` → 00

**Cálculo de Impostos**:
```
vBC = Valor do Produto + Frete
vICMS = vBC × aliq / 100
vPIS = vBC × taxa_padrao / 100
vCOFINS = vBC × taxa_padrao / 100
```

### Tipo: SAÍDA (Venda Normal)

**De onde vem**:
- CFOP: Determinado pelo `st` da NCM
  - Se `st = SIM` → CFOP 5403/6403
  - Se `st = NÃO` → CFOP 5102/6102
- CST ICMS:
  - Se `st = SIM` → 60
  - Se `st = NÃO` → 00

**Cálculo de Impostos** (mesmo que Entrada):
```
vBC = Valor do Produto + Frete
vICMS = vBC × aliq / 100
vPIS = vBC × taxa_padrao / 100
vCOFINS = vBC × taxa_padrao / 100
```

### Tipo: SAÍDA APÓS FUTURA (Entrega após Promessa)

**De onde vem**:
- CFOP: Sempre 5922/6922 (específico para este tipo)
- CST ICMS: Determinado pelo `st` da NCM
  - Se `st = SIM` → 60
  - Se `st = NÃO` → 00

**Cálculo de Impostos** (mesmo que Saída):
```
vBC = Valor do Produto + Frete
vICMS = vBC × aliq / 100
vPIS = vBC × taxa_padrao / 100
vCOFINS = vBC × taxa_padrao / 100
```

### Tipo: FUTURA (Venda com Promessa)

**De onde vem**:
- CFOP: Sempre 5117/6117
- CST ICMS: Determinado pelo `st` da NCM
  - Se `st = SIM` → 60
  - Se `st = NÃO` → 00

**Cálculo de Impostos** (TODOS ZERADOS):
```
vBC = 0.00
vICMS = 0.00
vPIS = 0.00
vCOFINS = 0.00
```

**Motivo**: Futura é uma promessa/booking, não é uma venda real. A tributação ocorre apenas na entrega (Saída Após Futura).

---

## 3. CST Codes Padrão

| Imposto | CST | Aplicação |
|---------|-----|-----------|
| ICMS | 00 ou 60 | Varia conforme `st` da NCM |
| IPI | 99 | Sempre |
| PIS | 01 | Sempre |
| COFINS | 01 | Sempre |

---

## 4. Caso Especial: Reposições

**Prática Atual**: Usuários preenchem produtos de reposição com valor de **R$ 0,01** (um centavo).

**O que o sistema faz**:
```
Valor: 0.01
NCM: Consultada normalmente
st: Determinado normalmente
vBC = 0.01 (ou proporcionado com frete)
vICMS = 0.01 × aliq / 100 (resultado mínimo)
vPIS = 0.01 × taxa / 100 (resultado mínimo)
vCOFINS = 0.01 × taxa / 100 (resultado mínimo)
```

**Validação Necessária**:
- ❓ Esta prática está correta fiscalmente?
- ❓ Deve-se usar R$ 0,00 ao invés?
- ❓ Qual é o procedimento correto para reposições?

---

## 5. Resumo da Lógica

| Tipo NFe | CFOP | CST ICMS | vBC | vICMS | vPIS | vCOFINS |
|----------|------|----------|-----|-------|------|---------|
| Entrada (Devolução) | De NCM (1411/1202) | De NCM (60/00) | Calculado | Calculado | Calculado | Calculado |
| Saída | De NCM (5403/5102) | De NCM (60/00) | Calculado | Calculado | Calculado | Calculado |
| Saída Após Futura | 5922/6922 | De NCM (60/00) | Calculado | Calculado | Calculado | Calculado |
| Futura | 5117/6117 | De NCM (60/00) | **0.00** | **0.00** | **0.00** | **0.00** |

---

## Questão para Contabilidade

**Reposições com R$ 0,01**: Validar se o procedimento atual está correto e orientar sobre a forma adequada de preenchimento.
