# Reforma Tributária 2025 - Lógica Fiscal do ERP Staccato

## Documento para Revisão Contábil

**Data:** 25 de Dezembro de 2025
**Versão:** 1.1
**Legislação Base:** Lei Complementar 214/2025
**Notas Técnicas:** NT 2024.002, NT 2025.002

---

## 1. Introdução

Este documento descreve a implementação da Reforma Tributária (LC 214/2025) no sistema ERP Staccato, destinado ao comércio de materiais de construção. O objetivo é permitir a revisão da lógica fiscal pelo contador responsável, garantindo conformidade com a legislação vigente.

### 1.1 Escopo da Implementação

O sistema implementa os seguintes novos tributos:

| Tributo | Nome Completo | Substitui |
|---------|---------------|-----------|
| **IBS** | Imposto sobre Bens e Serviços | ICMS + ISS |
| **CBS** | Contribuição sobre Bens e Serviços | PIS + COFINS |
| **IS** | Imposto Seletivo | Novo (produtos específicos) |

### 1.2 Cronograma de Transição

Conforme LC 214/2025, a transição ocorre gradualmente:

| Ano | Fator Novos Tributos | Fator Tributos Antigos | Observação |
|-----|---------------------|------------------------|------------|
| 2025 | 0% | 100% | Sistema antigo vigente |
| 2026 | Período de teste | - | Alíquotas fixas reduzidas |
| 2027 | 10% | 90% | Início da transição |
| 2028 | 20% | 80% | - |
| 2029 | 30% | 70% | - |
| 2030 | 40% | 60% | - |
| 2031 | 50% | 50% | - |
| 2032 | 90% | 10% | - |
| 2033+ | 100% | 0% | Sistema novo completo |

---

## 2. Estrutura dos Novos Tributos

### 2.1 IBS - Imposto sobre Bens e Serviços

O IBS é um imposto de competência compartilhada entre Estados (UF) e Municípios.

#### 2.1.1 Campos Implementados no Sistema

| Campo | Descrição | Tipo | Exemplo |
|-------|-----------|------|---------|
| `cstIBS` | Código de Situação Tributária | VARCHAR(3) | "000" |
| `cClassTribIBS` | Código de Classificação Tributária | VARCHAR(6) | "000001" |
| `vBCIBS` | Base de Cálculo do IBS | DECIMAL | R$ 1.000,00 |
| `pIBSUF` | Alíquota IBS Estadual | DECIMAL(5,4) | 12,0000% |
| `vTribOpIBSUF` | Valor do IBS Estadual | DECIMAL | R$ 120,00 |
| `pIBSMun` | Alíquota IBS Municipal | DECIMAL(5,4) | 5,7000% |
| `vTribOpIBSMun` | Valor do IBS Municipal | DECIMAL | R$ 57,00 |

#### 2.1.2 Alíquotas Finais do IBS (2033+)

| Componente | Alíquota | Dispositivo Legal |
|------------|----------|-------------------|
| IBS UF (Estadual) | 12,00% | LC 214/2025 Art. 156-A |
| IBS Municipal | 5,70% | LC 214/2025 Art. 156-A |
| **Total IBS** | **17,70%** | - |

#### 2.1.3 Alíquotas do Período de Teste (2026)

| Componente | Alíquota | Observação |
|------------|----------|------------|
| IBS UF | 0,10% | Teste do sistema |
| IBS Municipal | 0,00% | Municípios não participam em 2026 |
| **Total IBS** | **0,10%** | - |

**ATENÇÃO:** Estas alíquotas de teste devem ser verificadas contra a última publicação do Ministério da Fazenda.

---

### 2.2 CBS - Contribuição sobre Bens e Serviços

A CBS é uma contribuição federal que substitui PIS e COFINS.

#### 2.2.1 Campos Implementados no Sistema

| Campo | Descrição | Tipo | Exemplo |
|-------|-----------|------|---------|
| `cstCBS` | Código de Situação Tributária | VARCHAR(3) | "000" |
| `cClassTribCBS` | Código de Classificação Tributária | VARCHAR(6) | "000001" |
| `vBCCBS` | Base de Cálculo da CBS | DECIMAL | R$ 1.000,00 |
| `pCBS` | Alíquota CBS | DECIMAL(5,4) | 8,8000% |
| `vCBS` | Valor da CBS | DECIMAL | R$ 88,00 |
| `vTribOpCBS` | Valor da Operação Tributável CBS | DECIMAL | R$ 88,00 |

#### 2.2.2 Alíquotas da CBS

| Período | Alíquota | Observação |
|---------|----------|------------|
| 2026 (Teste) | 0,90% | Período de teste |
| 2033+ (Final) | 8,80% | Alíquota plena |

---

### 2.3 IS - Imposto Seletivo

O Imposto Seletivo incide sobre produtos considerados prejudiciais à saúde ou ao meio ambiente.

#### 2.3.1 Produtos Sujeitos ao IS

| NCM | Produto | Alíquota Estimada | Aplicável a Materiais de Construção? |
|-----|---------|-------------------|--------------------------------------|
| 2402-2403 | Cigarros e tabaco | 250% | **NÃO** |
| 2203-2208 | Bebidas alcoólicas | 46-62% | **NÃO** |
| 2202 | Bebidas açucaradas | 20% | **NÃO** |
| 8703, 8711 | Veículos automotores | 26,5% | **NÃO** |
| 8802, 8903 | Aeronaves e embarcações | 26,5% | **NÃO** |
| 2709-2711 | Combustíveis fósseis | 0,25-1% | **NÃO** |
| 2601 | Minerais extraídos | 1% | **NÃO** |

#### 2.3.2 Observação para Materiais de Construção

**IMPORTANTE:** Materiais de construção civil **NÃO** estão sujeitos ao Imposto Seletivo. O sistema mantém a estrutura de IS para compatibilidade, mas os produtos comercializados pela empresa não devem ter IS calculado.

Se algum produto aparecer com IS calculado, verificar se o NCM está configurado incorretamente na tabela de NCMs.

---

## 3. Códigos de Situação Tributária (CST)

### 3.1 CST para IBS e CBS

| CST | Descrição | Uso Comum |
|-----|-----------|-----------|
| 000 | Tributação integral | Operações normais |
| 010 | Tributação com redução de base de cálculo | Benefícios fiscais |
| 011 | Tributação com redução de alíquota | Regimes especiais |
| 200 | Tributação diferida | Diferimento |
| 210 | Tributação diferida com redução de BC | - |
| 220 | Tributação com suspensão | Exportação, ZFM |
| 221 | Tributação com suspensão e redução | - |
| 400 | Não tributada | Imunidades |
| 410 | Isenção | Isenções legais |
| 510 | Tributação monofásica | Combustíveis, medicamentos |
| 550 | Substituição tributária | ST |
| 800 | Crédito presumido | Regimes especiais |
| 810 | Transferência de crédito | - |
| 820 | Estorno de crédito | - |

### 3.2 Uso no Sistema

O sistema atualmente utiliza:
- **CST "000"** para todas as operações tributadas integralmente
- **CST vazio ("")** quando o tributo não se aplica (ex: IS para materiais de construção)

**RECOMENDAÇÃO:** Verificar se o uso de CST vazio está conforme as regras da SEFAZ. Pode ser necessário utilizar CST específico de não-incidência.

---

## 4. Códigos de Classificação Tributária (cClassTrib)

### 4.1 Estrutura do Código

O código de classificação tributária possui 6 dígitos no formato: `XXXXXX`

- **Primeiros 3 dígitos:** CST base
- **Últimos 3 dígitos:** Especificação adicional

### 4.2 Códigos Implementados no Sistema

#### 4.2.1 Códigos de Tributação Integral (CST 000)

| Código | Tipo | Descrição |
|--------|------|-----------|
| 000001 | IBS/CBS | Tributação integral - Alíquota padrão |
| 000002 | IBS/CBS | Tributação integral - Mercadorias |
| 000003 | IBS/CBS | Tributação integral - Serviços |

#### 4.2.2 Códigos com Redução (CST 010/011)

| Código | Tipo | Descrição | Redução |
|--------|------|-----------|---------|
| 010001 | IBS/CBS | Redução de BC - Cesta básica | 60% |
| 010002 | IBS/CBS | Redução de BC - Medicamentos | 60% |
| 010003 | IBS/CBS | Redução de BC - Educação | 60% |
| 010004 | IBS/CBS | Redução de BC - Saúde | 60% |
| 010005 | IBS/CBS | Redução de BC - Transporte coletivo | 60% |
| 200010 | IBS/CBS | **Materiais de construção (MCMV)** | **40%** |

#### 4.2.3 Códigos de Isenção/Não-Incidência (CST 400/410)

| Código | Tipo | Descrição |
|--------|------|-----------|
| 400001 | IBS/CBS | Não tributada - Imunidade constitucional |
| 410001 | IBS/CBS | Isento - Exportação |
| 410002 | IBS/CBS | Isento - Zona Franca de Manaus |

#### 4.2.4 Códigos para Imposto Seletivo (CST 620)

| Código | Tipo | Descrição |
|--------|------|-----------|
| 620001 | IS | Produtos de tabaco |
| 620002 | IS | Bebidas alcoólicas - Fermentadas |
| 620003 | IS | Bebidas alcoólicas - Destiladas |
| 620004 | IS | Bebidas açucaradas |
| 620005 | IS | Veículos automotores |
| 620006 | IS | Embarcações e aeronaves |
| 620007 | IS | Combustíveis fósseis |
| 620008 | IS | Minerais extraídos |

---

## 5. Cálculo dos Tributos

### 5.1 Fórmula de Cálculo - IBS

```
vBCIBS = Valor do Produto + Valor do Frete (proporcional)

vTribOpIBSUF = vBCIBS × pIBSUF / 100
vTribOpIBSMun = vBCIBS × pIBSMun / 100

Total IBS = vTribOpIBSUF + vTribOpIBSMun
```

### 5.2 Fórmula de Cálculo - CBS

```
vBCCBS = Valor do Produto + Valor do Frete (proporcional)

vCBS = vBCCBS × pCBS / 100
vTribOpCBS = vCBS
```

### 5.3 Fórmula de Cálculo - IS

```
vBCIS = Valor do Produto + Valor do Frete (proporcional)

vIS = vBCIS × pIS / 100
vTribOpIS = vIS
```

**Nota:** Para materiais de construção, o IS não deve ser calculado (alíquota = 0).

### 5.4 Base de Cálculo

A base de cálculo utilizada no sistema inclui:
- Valor total do produto
- Frete proporcional ao valor do produto

```
Frete Proporcional = (Valor Produto / Valor Total Produtos) × Valor Total Frete
Base de Cálculo = Valor Produto + Frete Proporcional
```

---

## 6. Integração com Tributos Antigos

### 6.1 Coexistência Durante a Transição

Durante o período de transição (2026-2032), o sistema deve calcular AMBOS os sistemas tributários:

| Tributo Antigo | Tributo Novo | Observação |
|----------------|--------------|------------|
| ICMS | IBS | Substituição gradual |
| PIS | CBS | Substituição gradual |
| COFINS | CBS | Substituição gradual |
| IPI | - | Continua para industrialização |

### 6.2 Campos Mantidos do Sistema Antigo

O sistema mantém todos os campos do regime tributário anterior:

- ICMS: `cstICMS`, `vBC`, `pICMS`, `vICMS`, etc.
- PIS: `cstPIS`, `vBCPIS`, `pPIS`, `vPIS`
- COFINS: `cstCOFINS`, `vBCCOFINS`, `pCOFINS`, `vCOFINS`
- ICMS-ST: `vBCST`, `pICMSST`, `vICMSST`, etc.

### 6.3 Lógica de Aplicação por Ano

| Ano | ICMS/PIS/COFINS | IBS/CBS | Observação |
|-----|-----------------|---------|------------|
| 2025 | 100% | 0% | Sistema atual |
| 2026 | 100% | Teste (1%) | Ambos calculados |
| 2027 | 90% | 10% | Transição |
| 2028 | 80% | 20% | Transição |
| 2029 | 70% | 30% | Transição |
| 2030 | 60% | 40% | Transição |
| 2031 | 50% | 50% | Transição |
| 2032 | 10% | 90% | Transição |
| 2033+ | 0% | 100% | Sistema novo |

**ATENÇÃO:** A lógica de aplicação proporcional (fator de transição) está definida no sistema mas **não está sendo aplicada automaticamente**. Atualmente, o sistema calcula os novos tributos em 100% independente do ano. Esta funcionalidade precisa ser revisada.

---

## 7. Tabela de NCM e Configurações

### 7.1 Campos da Tabela NCM

| Campo | Descrição | Uso |
|-------|-----------|-----|
| `ncm` | Código NCM (8 dígitos) | Identificador |
| `cest` | Código CEST (7 dígitos) | ST |
| `st` | Sujeito a ST | 0/1 |
| `mva4` | MVA para alíquota 4% | ST interestadual |
| `mva12` | MVA para alíquota 12% | ST interestadual |
| `aliq` | Alíquota ICMS interna | Cálculo ICMS |
| `cClassTribIBS` | Classificação IBS | Reforma Tributária |
| `cClassTribCBS` | Classificação CBS | Reforma Tributária |
| `sujeitoIS` | Sujeito a IS | 0/1 |
| `pIS` | Alíquota IS | Imposto Seletivo |
| `cClassTribIS` | Classificação IS | Imposto Seletivo |

### 7.2 Configuração Padrão

Para novos NCMs cadastrados, o sistema aplica os seguintes valores padrão:

| Campo | Valor Padrão | Significado |
|-------|--------------|-------------|
| `cClassTribIBS` | "000001" | Tributação integral |
| `cClassTribCBS` | "000001" | Tributação integral |
| `sujeitoIS` | 0 | Não sujeito a IS |
| `pIS` | 0.0 | Sem alíquota IS |

### 7.3 NCMs Típicos de Materiais de Construção

Os seguintes NCMs são comuns para materiais de construção e devem estar configurados corretamente:

| NCM | Descrição | ST | IS |
|-----|-----------|----|----|
| 6810.xx.xx | Artigos de cimento, concreto | Verificar | Não |
| 6811.xx.xx | Artigos de fibrocimento | Verificar | Não |
| 6901.xx.xx | Tijolos, ladrilhos cerâmicos | Verificar | Não |
| 6904.xx.xx | Tijolos de construção | Verificar | Não |
| 6905.xx.xx | Telhas cerâmicas | Verificar | Não |
| 6907.xx.xx | Ladrilhos e placas cerâmicas | Verificar | Não |
| 6910.xx.xx | Pias, lavatórios cerâmicos | Verificar | Não |
| 7213.xx.xx | Fio-máquina de ferro/aço | Verificar | Não |
| 7214.xx.xx | Barras de ferro/aço | Verificar | Não |
| 7217.xx.xx | Fios de ferro/aço | Verificar | Não |
| 7306.xx.xx | Tubos de ferro/aço | Verificar | Não |
| 7308.xx.xx | Construções de ferro/aço | Verificar | Não |

---

## 8. Regimes Especiais - Materiais de Construção

### 8.1 Programa Minha Casa Minha Vida

Conforme LC 214/2025 Art. 8º X, materiais de construção destinados ao programa Minha Casa Minha Vida podem ter redução de até 40% na base de cálculo do IBS e CBS.

**Status no Sistema:**
- Código de classificação `200010` está cadastrado
- A lógica de aplicação automática da redução **NÃO está implementada**
- Necessário aplicação manual ou desenvolvimento adicional

### 8.2 Zona Franca de Manaus

Operações destinadas à Zona Franca de Manaus possuem isenção de IBS e CBS.

**Status no Sistema:**
- Códigos de isenção estão cadastrados (410002)
- Aplicação deve ser feita manualmente no cadastro da NFe

---

## 9. Regras de Validação SEFAZ Implementadas

### 9.1 Base de Cálculo ICMS e CST 60

**Problema:** Para produtos com CST 60 (ICMS cobrado anteriormente por Substituição Tributária), o XML da NF-e **não inclui** o elemento `<vBC>` no bloco ICMS60. Portanto, o total de Base de Cálculo ICMS não deve somar estes itens.

**Implementação:**
```cpp
// Apenas soma vBC para CSTs que incluem base no XML
const QString cstICMS = modelProduto.data(row, "cstICMS").toString();
if (cstICMS != "60") {
    baseICMS += modelProduto.data(row, "vBC").toDouble();
}
```

**Regra SEFAZ:** "Total da BC ICMS difere do somatório dos itens" - Esta rejeição ocorre quando:
- Soma dos `<vBC>` dos itens ≠ `<vBC>` do bloco `<ICMSTot>`
- Para CST 60, não há `<vBC>` no item, logo não deve ser somado no total

### 9.2 Blocos IBS/CBS/IS Condicionais

**Problema:** Antes de 2026, as alíquotas de IBS/CBS são 0%. Enviar blocos IBSCBS com CST 000 (tributação integral) e alíquotas 0% causa rejeição: "Alíquota do IBS da UF inválida".

**Implementação:**
```cpp
// Só inclui blocos IBS/CBS quando há alíquotas > 0
const double pIBSUF = modelProduto.data(row, "pIBSUF").toDouble();
const double pIBSMun = modelProduto.data(row, "pIBSMun").toDouble();
const double pCBS = modelProduto.data(row, "pCBS").toDouble();
const bool hasIBSCBS = (pIBSUF > 0 || pIBSMun > 0 || pCBS > 0);

if (!cstIBS.isEmpty() && cstIBS != "0" && hasIBSCBS) {
    // Gera bloco IBSCBS
}
```

**Mesma lógica para IS:**
```cpp
const double pIS = modelProduto.data(row, "pIS").toDouble();
if (!cstIS.isEmpty() && cstIS != "0" && pIS > 0) {
    // Gera bloco ISel
}
```

**Resultado:**
- **2025:** Nenhum bloco IBS/CBS/IS é gerado (alíquotas = 0)
- **2026+:** Blocos são gerados apenas quando há tributação efetiva

---

## 10. Geração da NF-e

### 10.1 Blocos XML Novos

A NF-e com Reforma Tributária inclui os seguintes novos blocos:

```xml
<!-- Bloco IBS/CBS por produto -->
<IBSCBS>
    <CST>000</CST>
    <cClassTrib>000001</cClassTrib>
    <vBC>1000.00</vBC>
    <pIBSUF>12.00</pIBSUF>
    <vIBSUF>120.00</vIBSUF>
    <pIBSMun>5.70</pIBSMun>
    <vIBSMun>57.00</vIBSMun>
    <pCBS>8.80</pCBS>
    <vCBS>88.00</vCBS>
</IBSCBS>

<!-- Bloco IS por produto (quando aplicável) -->
<ISel>
    <CST>000</CST>
    <cClassTrib>620001</cClassTrib>
    <vBC>1000.00</vBC>
    <pIS>250.00</pIS>
    <vIS>2500.00</vIS>
</ISel>
```

### 10.2 Formato ACBr

O sistema gera comandos no formato ACBr INI:

```ini
[IBSCBS1]
CST=000
cClassTrib=000001
vBC=1000.00
pIBSUF=12.00
vIBSUF=120.00
pIBSMun=5.70
vIBSMun=57.00
pCBS=8.80
vCBS=88.00

[ISel1]
CST=000
cClassTrib=620001
vBC=1000.00
pIS=250.00
vIS=2500.00
```

---

## 11. Validações e Alertas

### 11.1 Validações Implementadas

| Validação | Descrição | Status |
|-----------|-----------|--------|
| Formato cClassTrib | 6 dígitos numéricos | Implementado |
| Existência cClassTrib | Código existe na tabela | Implementado |
| NCM 8 dígitos | NCM deve ter 8 dígitos | Implementado |
| CEST 7 dígitos | CEST deve ter 7 dígitos (se preenchido) | Implementado |

### 11.2 Validações Pendentes

| Validação | Descrição | Status |
|-----------|-----------|--------|
| cClassTrib vs NCM | Verificar compatibilidade | **Não implementado** |
| IS para não-sujeitos | Alertar se IS aplicado a material de construção | **Não implementado** |
| Redução MCMV | Verificar elegibilidade | **Não implementado** |
| Campos obrigatórios por CST | Verificar campos conforme NT SEFAZ | **Não implementado** |

---

## 12. Pontos de Atenção para o Contador

### 12.1 Verificações Necessárias

1. **Alíquotas de Teste 2026:**
   - Confirmar se 0,1% IBS + 0,9% CBS está conforme última orientação do Ministério da Fazenda
   - Verificar se há atualização nas alíquotas de teste

2. **Cronograma de Transição:**
   - Confirmar percentuais de cada ano contra resolução CONFAZ
   - Especial atenção ao salto 2031 (50%) → 2032 (90%)

3. **Materiais de Construção:**
   - Verificar se há NCMs com configuração incorreta de IS
   - Confirmar elegibilidade para redução MCMV

4. **Substituição Tributária:**
   - Verificar interação entre ST de ICMS e novos tributos
   - Confirmar tratamento para produtos com ST

### 12.2 Configurações Recomendadas

1. **Revisar todos os NCMs** cadastrados e confirmar:
   - `sujeitoIS = 0` para materiais de construção
   - `cClassTribIBS = "000001"` ou código específico se houver benefício
   - `cClassTribCBS = "000001"` ou código específico se houver benefício

2. **Documentar** qualquer NCM que tenha tratamento especial

3. **Testar** emissão de NFe em ambiente de homologação antes de 2026

---

## 13. Glossário

| Termo | Significado |
|-------|-------------|
| **BC** | Base de Cálculo |
| **CBS** | Contribuição sobre Bens e Serviços |
| **CEST** | Código Especificador da Substituição Tributária |
| **COFINS** | Contribuição para Financiamento da Seguridade Social |
| **CST** | Código de Situação Tributária |
| **IBS** | Imposto sobre Bens e Serviços |
| **ICMS** | Imposto sobre Circulação de Mercadorias e Serviços |
| **IS** | Imposto Seletivo |
| **LC** | Lei Complementar |
| **MCMV** | Minha Casa Minha Vida |
| **MVA** | Margem de Valor Agregado |
| **NCM** | Nomenclatura Comum do Mercosul |
| **NFe** | Nota Fiscal Eletrônica |
| **NT** | Nota Técnica |
| **PIS** | Programa de Integração Social |
| **SEFAZ** | Secretaria da Fazenda |
| **ST** | Substituição Tributária |
| **UF** | Unidade Federativa (Estado) |
| **ZFM** | Zona Franca de Manaus |

---

## 14. Referências Legais

1. **Lei Complementar 214/2025** - Institui o IBS, CBS e IS
2. **Nota Técnica 2024.002** - Leiaute NF-e versão 5.0
3. **Nota Técnica 2025.002** - Campos IBS/CBS/IS na NF-e
4. **Resolução CONFAZ** - Alíquotas e cronograma de transição
5. **Instrução Normativa RFB** - Regulamentação CBS

---

## 15. Histórico de Alterações

| Data | Versão | Alteração | Responsável |
|------|--------|-----------|-------------|
| 25/12/2025 | 1.0 | Versão inicial | Sistema |
| 25/12/2025 | 1.1 | Adicionada seção 9 com regras de validação SEFAZ (CST 60 e IBS/CBS condicionais) | Sistema |

---

## 16. Contato

Para dúvidas sobre a implementação técnica no sistema, entrar em contato com a equipe de desenvolvimento.

Para dúvidas sobre interpretação fiscal, consultar a legislação vigente e orientações da SEFAZ/RFB.

---

**AVISO LEGAL:** Este documento descreve a implementação técnica no sistema ERP e não constitui orientação fiscal ou contábil. A responsabilidade pela correta aplicação da legislação tributária é do contribuinte e seu contador. Recomenda-se sempre consultar a legislação vigente e, em caso de dúvida, buscar orientação junto aos órgãos competentes (SEFAZ, RFB).
