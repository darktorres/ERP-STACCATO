# Relatório: Lógica de Preenchimento de Impostos no Sistema ERP Staccato

**Data**: 19/01/2026
**Versão**: 1.0
**Destinatário**: Contabilidade
**Assunto**: Validação da lógica de cálculo e preenchimento de impostos nas Notas Fiscais Eletrônicas

---

## ⚠️ ATENÇÃO - QUESTÃO ABERTA PARA CONTABILIDADE

**Reposições com R$ 0,01**: O sistema atualmente permite que usuários preencham reposições (produtos com defeito devolvidos) com valor de **R$ 0,01 (um centavo)**.

**Questão**: É a forma correta de registrar reposições? Ver Seção 7 para análise da prática atual e solicitar orientação da contabilidade sobre o procedimento correto.

---

## Executive Summary

O sistema ERP Staccato preenche os impostos em Notas Fiscais Eletrônicas (NFes) seguindo rigorosamente as regulamentações da Receita Federal Brasileira. Este relatório descreve a lógica implementada para garantir conformidade fiscal e identifica problemas a corrigir.

**Pontos-chave**:
- ✅ Todos os dados de impostos são obrigatoriamente originários da tabela NCM
- ✅ Não existem valores padrão ou pressupostos - tudo vem do banco de dados
- ✅ Validações rigorosas impedem emissão de NFes incompletas
- ✅ Suporta 4 tipos de operações: Entrada, Saída, Saída Após Futura, e Venda com Promessa (Futura)
- ⚠️ Prática atual: Reposições preenchidas com R$ 0,01 - requer validação contábil (Seção 7)

---

## 1. Arquitetura da Lógica de Impostos

### 1.1 Origem dos Dados

Todos os valores de impostos vêm de uma única fonte: **Tabela NCM** (Nomenclatura Comum do Mercosul)

```
Produto → NCM (código do produto)
         ↓
    Tabela NCM (lookup)
         ↓
    Classificação Tributária (CST, alíquotas, classificações)
         ↓
    Preenchimento Automático de Impostos
```

### 1.2 Fluxo de Processamento

```
1. Usuário seleciona produtos e NFe tipo
            ↓
2. Sistema chama preencherImpostos()
            ↓
3. Para cada produto:
   a) Extrai NCM
   b) Consulta tabela NCM
   c) Valida dados obrigatórios
   d) Calcula impostos conforme tipo de operação
   e) Preenche campos da NFe
            ↓
4. Sistema valida dados completos
            ↓
5. NFe enviada para ACBr (gerador de XML)
```

---

## 2. Tabela NCM: Campos Obrigatórios

A tabela NCM deve estar **100% preenchida** para cada produto. Campos obrigatórios:

| Campo | Tipo | Descrição | Exemplo |
|-------|------|-----------|---------|
| `ncm` | VARCHAR(8) | Código NCM (8 dígitos) | `69072100` (Cerâmica) |
| `st` | BOOLEAN | Tem Substituição Tributária? | `1` (TRUE) ou `0` (FALSE) |
| `aliq` | DECIMAL(5,2) | Taxa ICMS (sistema antigo) | `7.00` (7%) |
| `cClassTribIBS` | VARCHAR(6) | Classificação IBS | `300001` |
| `cClassTribCBS` | VARCHAR(6) | Classificação CBS | `300001` |
| `sujeitoIS` | BOOLEAN | Sujeito a Imposto Seletivo? | `0` (FALSE) ou `1` (TRUE) |
| `pIS` | DECIMAL(5,2) | Taxa IS (Imposto Seletivo) | `10.50` (10.5%) |
| `cClassTribIS` | VARCHAR(6) | Classificação IS | `620001` |

**Validação**: Se qualquer campo obrigatório estiver vazio, o sistema rejeita a NFe com erro claro.

### 2.1 Exemplos de NCMs Cadastrados

**NCM 69072100 - Cerâmica (com ST)**:
```
st = 1 (TRUE)
aliq = 7.00%
cClassTribIBS = 300001
cClassTribCBS = 300001
sujeitoIS = 0 (FALSE)
pIS = 0.00
cClassTribIS = (vazio)
```

**NCM 39181000 - Vinil (sem ST, com IS)**:
```
st = 0 (FALSE)
aliq = 18.00%
cClassTribIBS = 100001
cClassTribCBS = 100001
sujeitoIS = 1 (TRUE)
pIS = 10.50%
cClassTribIS = 620001
```

---

## 3. Quatro Tipos de Operação

O sistema trata cada tipo de operação de forma diferente, conforme exigido pela legislação.

### 3.1 Tipo 1: ENTRADA (Compra de Fornecedor)

**CFOP Correspondente**: 1201/2201 (sem ST) ou 1411/2411 (com ST)

**Lógica**:

```
Para cada produto:
  1. Consulta NCM
  2. Verifica st (Substituição Tributária)

  Se st = 1 (com ST):
     CFOP = 1411 (mesma UF) ou 2411 (outra UF)
     CST ICMS = 60 (Substituição Tributária)

  Se st = 0 (sem ST):
     CFOP = 1202 (mesma UF) ou 2202 (outra UF)
     CST ICMS = 00 (Tributado integralmente)

  3. Calcula vBC = (valor produto + frete) * proporção
  4. Calcula vICMS = vBC * aliq / 100
  5. Calcula PIS e COFINS = vBC * taxa / 100
```

**Impostos Preenchidos**:
- ✅ ICMS: vBC, pICMS, vICMS, vBCST, pICMSST, vICMSST
- ✅ PIS: vBCPIS, pPIS, vPIS
- ✅ COFINS: vBCCOFINS, pCOFINS, vCOFINS
- ✅ Sistema Novo (IBS/CBS/IS): vBCIBS, pIBSUF, vIBSMun, vCBS, vIS

### 3.2 Tipo 2: SAÍDA (Venda Normal)

**CFOP Correspondente**: 5403/6403 (com ST) ou 5102/6102 (sem ST)

**Lógica**:
```
Para cada produto:
  1. Consulta NCM
  2. Verifica st

  Se st = 1 (com ST):
     CFOP = 5403 (mesma UF) ou 6403 (outra UF)
     CST ICMS = 60

  Se st = 0 (sem ST):
     CFOP = 5102 (mesma UF) ou 6102 (outra UF)
     CST ICMS = 00

  3. Calcula vBC = (valor produto + frete) * proporção
  4. Calcula vICMS = vBC * aliq / 100
     (Se interestadual sem ST: usa taxa de partilha)
  5. Calcula PIS e COFINS = vBC * taxa / 100
```

**Diferença da Entrada**: Usa CFOPs de saída e pode aplicar taxa de ICMS diferenciada em operações interestaduais.

### 3.3 Tipo 3: SAÍDA APÓS FUTURA (Entrega após Promessa)

**CFOP Correspondente**: 5922/6922

**Lógica**:
```
Para cada produto:
  1. Consulta NCM (mesmo padrão que Saída)
  2. Verifica st

  Se st = 1 (com ST):
     CST ICMS = 60

  Se st = 0 (sem ST):
     CST ICMS = 00

  3. Calcula impostos normalmente (IGUAL Saída)
     vBC, vICMS, vPIS, vCOFINS com valores reais

  4. Referencia a NFe Futura anterior via campo idNFeFutura
```

**Diferença**: CFOP específico (5922/6922) e referencia documento anterior.

### 3.4 Tipo 4: FUTURA (Venda com Promessa de Entrega)

**CFOP Correspondente**: 5117/6117
**Característica Principal**: Impostos = ZERO (é apenas uma promessa/booking)

**Lógica**:

```
Para cada produto:
  1. Consulta NCM
  2. Verifica st

  Se st = 1 (com ST):
     CST ICMS = 60

  Se st = 0 (sem ST):
     CST ICMS = 00

  3. PREENCHIMENTO CRÍTICO - TODOS OS IMPOSTOS = ZERO:
     vBC = 0.0           (base zerada)
     vICMS = 0.0         (imposto zerado)
     vPIS = 0.0
     vCOFINS = 0.0
     vIBS = 0.0          (novo sistema)
     vCBS = 0.0
     vIS = 0.0

  4. CST codes SÃO preenchidos (00 ou 60)
     MAS valores monetários SÃO ZERO
```

**Por que ZERO?** Conforme Resolução SEFAZ - SP (Consulta nº 11921/2016):
> "Na etapa de faturamento, emita uma nota fiscal de simples faturamento, SEM DESTAQUE DO ICMS. Quando a mercadoria for efetivamente entregue, emita a nota fiscal de saída COM VALORES FISCAIS e aplicação da alíquota correspondente."

**Propósito**: Evitar dupla tributação. A tributação ocorre apenas uma vez, na entrega real.

---

## 4. Códigos de Situação Tributária (CST)

O sistema preenche automaticamente os CST codes conforme o tipo de operação:

### 4.1 CST ICMS

| Situação | CST | Sistema | Produto |
|----------|-----|---------|---------|
| Tributação Normal | 00 | Qualquer | Sem ST |
| Substituição Tributária | 60 | Entrada/Saida/SaidaAposFutura/Futura | Com ST |

**Regra de Decisão**:
```
CST ICMS = (st == 1) ? "60" : "00"
```

### 4.2 CST IPI

| Situação | CST |
|----------|-----|
| Outros (padrão) | 99 |

**Constante**: Sempre "99" para todas as operações.

### 4.3 CST PIS

| Situação | CST |
|----------|-----|
| Tributação (padrão) | 01 |

**Constante**: Sempre "01" para todas as operações (quando produto sujeito a PIS).

### 4.4 CST COFINS

| Situação | CST |
|----------|-----|
| Tributação (padrão) | 01 |

**Constante**: Sempre "01" para todas as operações (quando produto sujeito a COFINS).

---

## 5. Validações Implementadas

O sistema possui validações em dois pontos: **preencimento** e **validação**.

### 5.1 Validações no Preenchimento (preencherImpostos)

```
Para CADA produto:
  ✓ NCM existe na tabela? Senão → ERRO
  ✓ NCM tem cClassTribIBS preenchido? Senão → ERRO
  ✓ NCM tem cClassTribCBS preenchido? Senão → ERRO
  ✓ Se produtoIS=TRUE, tem cClassTribIS? Senão → ERRO
```

**Mensagens de Erro**:
```
"NCM 69072100 não encontrado na tabela NCM!"
"NCM 69072100: cClassTribIBS não preenchida!"
"NCM 69072100: cClassTribCBS não preenchida!"
"NCM 69072100: cClassTribIS não preenchida (produto sujeito a IS)!"
```

### 5.2 Validações na Confirmação (validarDados)

Antes de enviar para ACBr, o sistema valida:

**Para FUTURA especificamente**:
```
✓ CST ICMS preenchido? (00 ou 60)
✓ CST PIS preenchido? (01)
✓ CST COFINS preenchido? (01)
✓ vBC == 0.0? (deve ser zero)
✓ vICMS == 0.0? (deve ser zero)
✓ vPIS == 0.0? (deve ser zero)
✓ vCOFINS == 0.0? (deve ser zero)
```

**Mensagens de Erro para Futura**:
```
"Linha 1: CST ICMS não preenchido (Futura NFe)!"
"Linha 1: vBC deve ser 0 em Futura NFe!"
"Linha 1: vICMS deve ser 0 em Futura NFe!"
```

---

## 6. Exemplo Prático Completo

### 6.1 Cenário: Venda de Cerâmica (NCM 69072100)

**Dados da NFe**:
- Tipo: SAÍDA (venda normal)
- Produto: Cerâmica
- NCM: 69072100
- Quantidade: 14 m²
- Valor Unitário: R$ 144,29
- Valor Total: R$ 2.020,07
- Frete: R$ 112,20
- UF Origem: SP
- UF Destino: SP (mesma UF)

**Consulta NCM**:
```
SELECT st, aliq, cClassTribIBS, cClassTribCBS, sujeitoIS, pIS, cClassTribIS
FROM ncm WHERE ncm = '69072100'

Resultado:
st = 1 (TRUE - tem ST)
aliq = 7.00%
cClassTribIBS = 300001
cClassTribCBS = 300001
sujeitoIS = 0 (FALSE - não sujeito a IS)
pIS = 0.00
cClassTribIS = (vazio)
```

**Preenchimento Automático**:

```
Passo 1: Determinação de CFOP e CST
  st = 1 (tem ST)
  → CFOP = 5403 (mesma UF, com ST)
  → CST ICMS = 60

Passo 2: Cálculo de Base e Impostos
  vTotal = 2.020,07
  vFrete = 112,20
  vBC = 2.020,07 + 112,20 = 2.132,27

  pICMS = 7.00% (de NCM.aliq)
  vICMS = 2.132,27 × 7% = 149,26

  pPIS = 1.65% (taxa padrão)
  vPIS = 2.132,27 × 1.65% = 35,18

  pCOFINS = 7.60% (taxa padrão)
  vCOFINS = 2.132,27 × 7.60% = 162,05

Passo 3: Novo Sistema Tributário (IBS/CBS/IS)
  cClassTribIBS = 300001 (de NCM)
  cClassTribCBS = 300001 (de NCM)

  Alíquotas Reforma Tributária (jan/2026):
  pIBSUF = 7.34%
  pIBSMun = 0.00%
  pCBS = 2.10%

  vTribOpIBSUF = 2.132,27 × 7.34% = 156,51
  vCBS = 2.132,27 × 2.10% = 44,78

Passo 4: CST Padrão
  cstIPI = 99
  cstPIS = 01
  cstCOFINS = 01
```

**XML Final (simplificado)**:
```xml
<det nItem="001">
  <prod>
    <ncm>69072100</ncm>
    <cfop>5403</cfop>
  </prod>
  <imposto>
    <ICMS>
      <ICMS60>
        <CST>60</CST>
        <vBC>2132.27</vBC>
        <pICMS>7.00</pICMS>
        <vICMS>149.26</vICMS>
      </ICMS60>
    </ICMS>
    <PIS>
      <PISAliq>
        <CST>01</CST>
        <vBC>2132.27</vBC>
        <pPIS>1.65</pPIS>
        <vPIS>35.18</vPIS>
      </PISAliq>
    </PIS>
    <COFINS>
      <COFINSAliq>
        <CST>01</CST>
        <vBC>2132.27</vBC>
        <pCOFINS>7.60</pCOFINS>
        <vCOFINS>162.05</vCOFINS>
      </COFINSAliq>
    </COFINS>
    <IBS>
      <vBCIBS>2132.27</vBCIBS>
      <pIBSUF>7.34</pIBSUF>
      <vTribOpIBSUF>156.51</vTribOpIBSUF>
      <pCBS>2.10</pCBS>
      <vCBS>44.78</vCBS>
    </IBS>
  </imposto>
</det>
```

---

## 7. Caso Especial: NFes de Reposição (Prática Atual)

### 7.1 Como é Feito Atualmente

**Situação**: Quando um produto é uma reposição (substituição de produto com defeito, sem cobança), o usuário preenche manualmente o valor de **R$ 0,01 (um centavo)**.

**Motivação da prática atual**:
- Não é possível preencher com R$ 0,00 (zero) no sistema de forma normal
- Usar R$ 0,01 permite registrar o movimento na NFe
- Possibilita rastreamento do produto reposto
- Minimiza o impacto fiscal (impostos praticamente zero)

### 7.2 O que Acontece com R$ 0,01

**Preenchimento do usuário**:
```
Produto: BATTUTO GR NAT 100X100 RET LOES (com defeito)
NCM: 69072100
Quantidade: 14 m²
ValorUnitario: 0.01
ValorTotal: 0.14 (14 × R$ 0,01)

Consulta NCM: 69072100
st = 1 (com ST)
aliq = 7.00%
```

**Impostos Calculados pelo Sistema**:
```
vBC = 0.14 (valor + frete proporcionado)
vICMS = 0.14 × 7% = 0.0098 ≈ 0.01
vPIS = 0.14 × 1.65% = 0.0023 ≈ 0.00
vCOFINS = 0.14 × 7.60% = 0.0106 ≈ 0.01

CST ICMS = 60 (conforme NCM st=1)
```

**XML Gerado**:
```xml
<det nItem="001">
  <prod>
    <ncm>69072100</ncm>
    <vItem>0.14</vItem>
  </prod>
  <imposto>
    <ICMS>
      <ICMS60>
        <CST>60</CST>
        <vBC>0.14</vBC>
        <pICMS>7.00</pICMS>
        <vICMS>0.01</vICMS>
      </ICMS60>
    </ICMS>
  </imposto>
</det>
```

### 7.3 Questões para Contabilidade Validar

**Esta prática precisa ser revisada pela contabilidade**:

1. ❓ Usar R$ 0,01 é a forma correta de registrar reposições?
2. ❓ Qual CFOP deve ser utilizado para reposições?
3. ❓ Os impostos calculados (mesmo que mínimos) estão corretos?
4. ❓ Deve haver referência ao documento original (NFe com defeito)?
5. ❓ Existe forma alternativa de registrar (R$ 0,00, memo contábil, etc)?
6. ❓ Como tratar a reposição no lançamento contábil?

### 7.4 Legislação Relevante

Possíveis referências para consulta:

- **RICMS-SP** - Regulamento do ICMS de São Paulo
- **SEFAZ** - Secretaria da Fazenda
- **Código Fiscal de Operações (CFOP)** - Códigos de operação para devolução/reposição
- **Resolução SEFAZ** - Procedimentos para operações com valor zero ou muito reduzido
- **Instrução Normativa** - Normas sobre substituição de mercadoria

### 7.5 Impacto Atual

**Contábil**:
```
Débito: Devolução/Reposição         R$   0,14
Crédito: Vendas                      R$   0,14
```

**Fiscal**:
- ICMS: R$ 0,01 (mínimo)
- PIS: R$ 0,00
- COFINS: R$ 0,01 (mínimo)
- Total de impostos: ≈ R$ 0,02

### 7.6 O Sistema Atualmente

- ✅ Permite preenchimento com R$ 0,01
- ✅ Calcula impostos normalmente (proporcionalmente)
- ✅ Gera XML válido
- ❓ **Conformidade fiscal: REQUER VALIDAÇÃO DA CONTABILIDADE**

---

## 8. Comparativo: Os 4 Tipos de Operação

| Aspecto | Entrada | Saída | Saída Após Futura | Futura |
|---------|---------|-------|-------------------|--------|
| **CFOP** | 1411/2411 (ST) ou 1202/2202 | 5403/6403 (ST) ou 5102/6102 | 5922/6922 | 5117/6117 |
| **CST ICMS** | 00 ou 60 (de NCM) | 00 ou 60 (de NCM) | 00 ou 60 (de NCM) | 00 ou 60 (de NCM) |
| **vBC** | Calculado | Calculado | Calculado | **ZERO** |
| **vICMS** | Calculado | Calculado | Calculado | **ZERO** |
| **vPIS** | Calculado | Calculado | Calculado | **ZERO** |
| **vCOFINS** | Calculado | Calculado | Calculado | **ZERO** |
| **Propósito** | Registrar compra | Tributação real | Tributação real | Booking/Promessa |
| **Referencia** | - | - | NFe Futura | - |

---

## 9. Conformidade Fiscal

### 8.1 Referências Legais

Este sistema segue as seguintes disposições:

1. **Lei nº 9.638/98** (Lei da NFe)
2. **Resolução SEFAZ - SP Consulta nº 11921/2016** (Venda Futura)
3. **Decreto nº 29.937/2009** - Tabela NCM
4. **Reforma Tributária - Lei Complementar nº 214/2025** (Sistema IBS/CBS/IS)
5. **Instruções SEFAZ** - Padrão de Transmissão de NFe v4.00

### 8.2 Checklist de Conformidade

- ✅ NCM obrigatoriamente consultado para cada produto
- ✅ Dados de NCM são únicos ponto de entrada de classificação fiscal
- ✅ Sem valores padrão - tudo vem de dados cadastrados
- ✅ CST determinado automaticamente (sem intervenção manual)
- ✅ Base de cálculo sempre respeitada
- ✅ Impostos zerados apenas para Futura (conforme legislação)
- ✅ Referência de documento anterior para Saída Após Futura
- ✅ Validações impedem emissão incompleta

---

## 10. Responsabilidades

### 9.1 Contabilidade

- ✓ Manter tabela NCM 100% preenchida
- ✓ Revisar e validar dados de NCM em auditoria
- ✓ Acompanhar mudanças nas alíquotas
- ✓ Validar CST codes conforme SEFAZ
- ✓ Garantir conformidade mensal

### 9.2 Sistema ERP

- ✓ Consultar NCM para cada produto
- ✓ Validar dados obrigatórios antes de preenchimento
- ✓ Calcular impostos conforme tipo de operação
- ✓ Gerar XML de acordo com padrão SEFAZ
- ✓ Rejeitar dados incompletos com mensagens claras

### 9.3 Auditoria

- ✓ Verificar preenchimento correto de NCM
- ✓ Auditar CST codes utilizados
- ✓ Validar alíquotas contra legislação atual
- ✓ Revisar valores de impostos zerados (Futura)

---

## 11. Conclusão

O sistema ERP Staccato implementa uma lógica de preenchimento de impostos **rigorosamente baseada em dados** e **totalmente validada**, garantindo conformidade fiscal e evitando erros de lançamento.

**Principais benefícios**:
1. Automatização reduz erros manuais
2. Rastreabilidade completa (tudo vem de NCM)
3. Validações obrigatórias garantem integridade
4. Suporta 4 tipos de operação conforme legislação
5. Implementa novo sistema tributário (IBS/CBS/IS)

**Questão Aberta para Contabilidade**:
- ⚠️ Validar se a prática atual de preenchimento de reposições com R$ 0,01 está correta
- ⚠️ Definir procedimento correto para reposições na NFe (conforme Seção 7)
- ⚠️ Implementar validação no sistema após orientação contábil

---

**Aprovação Recomendada**: ✓ Compatível com regulamentações fiscais brasileiras

---

**Anexos**:
- Diagrama de fluxo de preenchimento
- Estrutura da tabela NCM (DDL)
- Exemplos de XML gerados
- Referências legais completas
