# Análise do Bug: Valores Incorretos no Orçamento

## Descrição do Problema

O sistema apresentava divergências entre os valores calculados e os valores exibidos nas spinboxes do orçamento, resultando no erro:
```
'Erro nos valores! Entre em contato com o suporte!'
```

Os logs mostravam discrepâncias como:
- `subTotalLiq: 20374.8` vs `spinBoxLiq: 20549.8`
- `total: 20374.8` vs `spinBoxTotal: 22224`

## Causa Raiz

O problema estava na **inconsistência de estado** entre duas funções que calculam totais de formas diferentes:

1. **`calcPrecoGlobalTotal()`** - Calcula valores a partir dos dados brutos e atualiza as spinboxes
2. **`calcularTotais()`** - Lê valores já salvos nos campos individuais dos itens (`parcial`, `parcialDesc`, `total`)
3. **`verificarTotais()`** - Compara os dois métodos e detecta discrepâncias

### O Bug Específico

A função `calcPrecoGlobalTotal()` **não estava atualizando** os campos individuais dos itens no banco de dados:
- `parcial` (valor bruto do item)  
- `parcialDesc` (valor com desconto individual)
- `total` (valor final com desconto global)

Isso causava uma situação onde:
- ✅ **Spinboxes exibiam valores corretos** (atualizados por `calcPrecoGlobalTotal()`)
- ❌ **Campos individuais continham valores desatualizados** (não sincronizados)

Quando `verificarTotais()` comparava os totais, encontrava divergência e gerava o erro.

## Passo-a-passo para Reproduzir o Bug

### 1. Criar um novo orçamento
- Abrir a tela de orçamento
- Criar um novo orçamento
- Definir um cliente

### 2. Adicionar itens ao orçamento
- Adicionar vários produtos com diferentes preços unitários
- Definir quantidades para cada item
- Aplicar descontos individuais em alguns itens (campo "desconto")

### 3. Aplicar desconto global
- Definir um desconto global no campo `doubleSpinBoxDescontoGlobal` (ex: 10%)
- Isso aciona `on_doubleSpinBoxDescontoGlobal_valueChanged()`

### 4. Modificar valores de frete
- Alterar o valor do frete no campo `doubleSpinBoxFrete`
- Isso aciona `on_doubleSpinBoxFrete_valueChanged()`

### 5. O problema ocorre aqui:
- `calcPrecoGlobalTotal()` é chamada e atualiza corretamente as spinboxes
- **MAS** não atualiza os campos individuais dos itens no modelo

### 6. Tentar salvar o orçamento
- Clicar em "Salvar" ou "Atualizar Orçamento"
- Isso chama `save()` que executa `verificarTotais()`

### 7. A divergência é detectada:
- `verificarTotais()` chama `calcularTotais()` que lê os campos individuais desatualizados
- Encontra divergência entre valores das spinboxes vs soma dos campos individuais

### 8. Sistema tenta corrigir automaticamente:
- Chama `corrigirValores()` para sincronizar
- Chama `verificarTotais()` novamente

### 9. Se ainda houver divergência:
- Gera o erro: **"Erro nos valores! Entre em contato com o suporte!"**
- Salva log detalhado com as diferenças

## Condições que Facilitam a Reprodução

- **Orçamentos com muitos itens** (maior chance de erros de arredondamento)
- **Descontos globais aplicados** (requer recálculo de todos os itens)  
- **Modificações frequentes nos valores** (frete, descontos)
- **Itens com preços decimais complexos** (maior chance de problemas de precisão)

## Solução Implementada

A correção foi feita na função `calcPrecoGlobalTotal()` em `src/orcamento.cpp:801`:

### Antes (código problemático):
```cpp
void Orcamento::calcPrecoGlobalTotal() {
  double subTotalBruto = 0.;
  double subTotalItens = 0.;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; }

    const double itemBruto = modelItem.data(row, "quant").toDouble() * modelItem.data(row, "prcUnitario").toDouble();
    const double descItem = modelItem.data(row, "desconto").toDouble() / 100.;
    const double stItem = itemBruto * (1. - descItem);
    subTotalBruto += itemBruto;
    subTotalItens += stItem;
  }

  ui->doubleSpinBoxSubTotalBruto->setValue(subTotalBruto);
  ui->doubleSpinBoxSubTotalLiq->setValue(subTotalItens);
  // ... resto da função sem atualizar campos individuais
}
```

### Depois (código corrigido):
```cpp
void Orcamento::calcPrecoGlobalTotal() {
  double subTotalBruto = 0.;
  double subTotalItens = 0.;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; }

    const double quant = modelItem.data(row, "quant").toDouble();
    const double prcUnitario = modelItem.data(row, "prcUnitario").toDouble();
    const double descItem = modelItem.data(row, "desconto").toDouble() / 100.;
    
    const double itemBruto = quant * prcUnitario;
    const double stItem = itemBruto * (1. - descItem);
    
    // ✅ CORREÇÃO: Atualiza os campos individuais dos itens
    modelItem.setData(row, "parcial", itemBruto);
    modelItem.setData(row, "parcialDesc", stItem);
    
    subTotalBruto += itemBruto;
    subTotalItens += stItem;
  }

  // ... código para atualizar spinboxes ...

  // ✅ CORREÇÃO: Atualiza o campo 'total' aplicando desconto global
  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; }
    
    const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
    const double descGlobal = modelItem.data(row, "descGlobal").toDouble() / 100.;
    modelItem.setData(row, "total", parcialDesc * (1 - descGlobal));
  }
}
```

## Mudanças Principais

1. **Sincronização de campos individuais**: Agora `calcPrecoGlobalTotal()` atualiza os campos `parcial` e `parcialDesc` de cada item
2. **Atualização do campo total**: Adiciona um segundo loop para atualizar o campo `total` de cada item aplicando o desconto global
3. **Consistência de estado**: Garante que spinboxes e campos individuais sempre estejam sincronizados

## Resultado Esperado

Após a correção:
- ✅ Eliminação das divergências entre valores calculados e exibidos
- ✅ Fim dos erros "Erro nos valores! Entre em contato com o suporte!"
- ✅ Consistência completa entre interface e dados subjacentes
- ✅ Cálculos corretos em todos os cenários (descontos, frete, múltiplos itens)

## Arquivos Modificados

- `src/orcamento.cpp` (linhas 801-848)

## Teste de Regressão Recomendado

1. Criar orçamentos com múltiplos itens e diferentes configurações de desconto
2. Aplicar descontos globais e individuais
3. Modificar valores de frete
4. Verificar se valores permanecem consistentes ao salvar
5. Testar cenários com muitos itens e valores decimais complexos