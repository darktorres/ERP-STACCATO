# -*- coding: utf-8 -*-
"""Gera db/seed_permissoes.sql a partir de ui/*.ui.

Escrever centenas de INSERTs a mao e divergencia garantida entre o catalogo e o que existe na tela,
entao o catalogo e derivado do fonte. Reexecutar depois de mexer nos .ui regenera o seed; todo INSERT
do catalogo/perfis usa insert-only (ON DUPLICATE KEY UPDATE que nao muda 'permitido' de linha ja
existente) e preserva o que ja foi configurado em producao via Gerenciar Permissoes.

Rodar com:  python db/gerar_catalogo_permissoes.py                       (rotina, seguro reexecutar)
            python db/gerar_catalogo_permissoes.py --com-paridade-legado (SO no corte inicial)
(python, nao python3)

--com-paridade-legado liga o bloco 3 (PARIDADE DAS CHAVES LEGADAS): backfill de perfil_has_permissao
e usuario_has_permissao_override a partir da tabela antiga usuario_has_permissao, para ninguem perder
nem ganhar acesso no dia do corte. Esse bloco LE de usuario_has_permissao e SOBRESCREVE
incondicionalmente (nao e insert-only) - correto na hora do corte, mas reexecutar depois disso
reverteria qualquer ajuste feito em producao nessas 19 chaves para o valor congelado da tabela antiga.
Por isso fica atras da flag: o modo padrao (sem flag), usado no dia a dia para regenerar o catalogo
apos mexer num .ui, nunca toca nele.

REGRA DO SEED: toda chave nasce PERMITIDA para os 10 tipos, e so recebe 0 onde existe bloqueio HOJE.
Derivar concessao de view_tab_* nao funciona - ele cobre 12 chaves, e quase metade do catalogo vive
em dialogos que nao descendem de aba nenhuma (searchdialog, inputdialog, followup, orcamento.ui,
venda.ui). Zera-las travaria o sistema inteiro no dia 1. O INVENTARIO abaixo e fechado e foi
levantado do fonte (107 ocorrencias varridas em src/), nao deduzido.
"""

import glob
import io
import os
import re
import sys
from collections import OrderedDict

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UI = os.path.join(RAIZ, "ui")
SAIDA = os.path.join(RAIZ, "db", "seed_permissoes.sql")

# --------------------------------------------------------------------------------------------------
# Mapa arquivo .ui -> (modulo, tela, prefixo tecnico da chave). Unica parte manual do gerador.
#
# O prefixo e EXPLICITO de proposito. Derivar do nome do arquivo produz colisao (compraavulsa.ui x
# widgetcompraavulsa.ui, devolucao.ui x widgetdevolucao.ui, estoque.ui x tabestoque.ui, tabgalpao.ui
# x widgetgalpao.ui) e, pior, geraria 'logisticaentregas.cancelarEntrega' em vez de
# 'logistica.entregas.cancelarEntrega' - a permissao que motivou o projeto ficaria sem efeito.
# --------------------------------------------------------------------------------------------------
MAPA = OrderedDict([
    ("mainwindow.ui",                    (u"Menu",          u"Menu",                  u"menu")),

    ("widgetorcamento.ui",               (u"Orçamentos",   u"Orçamentos",           u"orcamento.lista")),
    ("orcamento.ui",                     (u"Orçamentos",   u"Orçamento",            u"orcamento.orcamento")),
    ("baixaorcamento.ui",                (u"Orçamentos",   u"Baixa",                 u"orcamento.baixa")),

    ("widgetvenda.ui",                   (u"Vendas",        u"Vendas",                u"vendas.lista")),
    ("venda.ui",                         (u"Vendas",        u"Venda",                 u"vendas.venda")),
    ("devolucao.ui",                     (u"Vendas",        u"Devolução",            u"vendas.devolucao")),
    ("cancelaproduto.ui",                (u"Vendas",        u"Cancelar produto",      u"vendas.cancelarProduto")),

    ("tabcompras.ui",                    (u"Compras",       u"Compras",               u"compras")),
    ("widgetcompraresumo.ui",            (u"Compras",       u"Resumo",                u"compras.resumo")),
    ("widgetcomprapendentes.ui",         (u"Compras",       u"Pendentes",             u"compras.pendentes")),
    ("widgetcompragerar.ui",             (u"Compras",       u"Gerar Compra",          u"compras.comprar")),
    ("widgetcompraconfirmar.ui",         (u"Compras",       u"Confirmar Compra",      u"compras.confirmar")),
    ("widgetcomprafaturar.ui",           (u"Compras",       u"Faturamento",           u"compras.faturamento")),
    ("widgetcompraconsumos.ui",          (u"Compras",       u"Consumos",              u"compras.consumos")),
    ("widgetcomprahistorico.ui",         (u"Compras",       u"Histórico",            u"compras.historico")),
    ("widgetcompradevolucao.ui",         (u"Compras",       u"Devoluções",           u"compras.devolucoes")),
    ("widgetcompraavulsa.ui",            (u"Compras",       u"Avulso",                u"compras.avulso")),
    ("compraavulsa.ui",                  (u"Compras",       u"Compra avulsa",         u"compras.compraAvulsa")),
    ("produtospendentes.ui",             (u"Compras",       u"Produtos pendentes",    u"compras.produtosPendentes")),

    ("tablogistica.ui",                  (u"Logística",    u"Logística",            u"logistica")),
    ("widgetlogisticaagendarcoleta.ui",  (u"Logística",    u"Agendar Coleta",        u"logistica.agendarColeta")),
    ("widgetlogisticacoleta.ui",         (u"Logística",    u"Coleta",                u"logistica.coleta")),
    ("widgetlogisticarecebimento.ui",    (u"Logística",    u"Recebimento",           u"logistica.recebimento")),
    ("widgetlogisticaagendarentrega.ui", (u"Logística",    u"Agendar Entrega",       u"logistica.agendarEntrega")),
    ("widgetlogisticaseparacao.ui",      (u"Logística",    u"Separação",             u"logistica.separacao")),
    ("widgetlogisticaentregas.ui",       (u"Logística",    u"Entregas",              u"logistica.entregas")),
    ("widgetlogisticacaminhao.ui",       (u"Logística",    u"Caminhões",            u"logistica.caminhoes")),
    ("widgetlogisticarepresentacao.ui",  (u"Logística",    u"Representação",        u"logistica.representacao")),
    ("widgetlogisticaentregues.ui",      (u"Logística",    u"Entregues",             u"logistica.entregues")),
    ("widgetlogisticacalendario.ui",     (u"Logística",    u"Calendário",           u"logistica.calendario")),
    ("widgetdevolucao.ui",               (u"Logística",    u"Devolução",            u"logistica.devolucao")),

    ("tabnfe.ui",                        (u"NF-e",          u"NF-e",                  u"nfe")),
    ("widgetnfeentrada.ui",              (u"NF-e",          u"Entrada",               u"nfe.entrada")),
    ("widgetnfesaida.ui",                (u"NF-e",          u"Saída",                u"nfe.saida")),
    ("widgetnfedistribuicao.ui",         (u"NF-e",          u"Distribuição",         u"nfe.distribuicao")),
    ("cadastrarnfe.ui",                  (u"NF-e",          u"Cadastrar NF-e",        u"nfe.cadastrarNFe")),
    ("importarxml.ui",                   (u"NF-e",          u"Importar XML",          u"nfe.importarXml")),
    ("xml_viewer.ui",                    (u"NF-e",          u"Visualizar XML",        u"nfe.visualizarXml")),

    ("tabestoque.ui",                    (u"Estoque",       u"Estoque",               u"estoque")),
    ("widgetestoques.ui",                (u"Estoque",       u"Estoques",              u"estoque.estoques")),
    ("widgetestoqueproduto.ui",          (u"Estoque",       u"Produtos",              u"estoque.produtos")),
    ("estoque.ui",                       (u"Estoque",       u"Estoque (item)",        u"estoque.item")),

    ("tabgalpao.ui",                     (u"Galpão",       u"Galpão",               u"galpao")),
    ("widgetgalpao.ui",                  (u"Galpão",       u"Galpão",               u"galpao.galpao")),
    ("widgetgalpaopeso.ui",              (u"Galpão",       u"Peso",                  u"galpao.peso")),

    ("tabfinanceiro.ui",                 (u"Financeiro",    u"Financeiro",            u"financeiro")),
    ("widgetfinanceirofluxocaixa.ui",    (u"Financeiro",    u"Fluxo de Caixa",        u"financeiro.fluxoCaixa")),
    ("widgetfinanceirocontas.ui",        (u"Financeiro",    u"Contas a Pagar",        u"financeiro.contasPagar")),
    ("widgetgare.ui",                    (u"Financeiro",    u"GARE",                  u"financeiro.gare")),
    ("widgetfinanceirocompra.ui",        (u"Financeiro",    u"Compras",               u"financeiro.compra")),
    ("widgetpagamentos.ui",              (u"Financeiro",    u"Pagamentos",            u"financeiro.pagamentos")),
    ("contas.ui",                        (u"Financeiro",    u"Contas (diálogo)",     u"financeiro.contasDialogo")),
    ("inserirlancamento.ui",             (u"Financeiro",    u"Inserir lançamento",   u"financeiro.inserirLancamento")),
    ("inserirtransferencia.ui",          (u"Financeiro",    u"Inserir transferência", u"financeiro.inserirTransferencia")),
    ("anteciparrecebimento.ui",          (u"Financeiro",    u"Antecipar recebimento", u"financeiro.anteciparRecebimento")),
    ("comprovantes.ui",                  (u"Financeiro",    u"Comprovantes",          u"financeiro.comprovantes")),
    ("pagamentosdia.ui",                 (u"Financeiro",    u"Pagamentos do dia",     u"financeiro.pagamentosDia")),

    ("widgetrelatorio.ui",               (u"Relatórios",   u"Relatórios",           u"relatorios")),
    ("widgetgraficos.ui",                (u"Gráficos",     u"Gráficos",             u"graficos")),
    ("widgetrh.ui",                      (u"RH",            u"RH",                    u"rh")),
    ("widgetconsistencia.ui",            (u"Consistência", u"Consistência",         u"consistencia")),

    ("cadastrocliente.ui",               (u"Cadastros",     u"Cliente",               u"cadastros.cliente")),
    ("cadastrofornecedor.ui",            (u"Cadastros",     u"Fornecedor",            u"cadastros.fornecedor")),
    ("cadastrofuncionario.ui",           (u"Cadastros",     u"Funcionário",          u"cadastros.funcionario")),
    ("cadastroloja.ui",                  (u"Cadastros",     u"Loja",                  u"cadastros.loja")),
    ("cadastroncm.ui",                   (u"Cadastros",     u"NCM",                   u"cadastros.ncm")),
    ("cadastropagamento.ui",             (u"Cadastros",     u"Pagamento",             u"cadastros.pagamento")),
    ("cadastroproduto.ui",               (u"Cadastros",     u"Produto",               u"cadastros.produto")),
    ("cadastroprofissional.ui",          (u"Cadastros",     u"Profissional",          u"cadastros.profissional")),
    ("cadastrotransportadora.ui",        (u"Cadastros",     u"Transportadora",        u"cadastros.transportadora")),
    ("cadastrousuario.ui",               (u"Cadastros",     u"Usuário",              u"cadastros.usuario")),
    ("cadastroStaccatoOff.ui",           (u"Cadastros",     u"Staccato OFF",          u"cadastros.staccatoOff")),
    ("precoestoque.ui",                  (u"Cadastros",     u"Preço estoque",        u"cadastros.precoEstoque")),
    ("importaprodutos.ui",               (u"Cadastros",     u"Importar produtos",     u"cadastros.importarProdutos")),
    ("calculofrete.ui",                  (u"Cadastros",     u"Calcular frete",        u"cadastros.calculoFrete")),

    ("userconfig.ui",                    (u"Sistema",       u"Configurações",        u"sistema.config")),
    ("searchdialog.ui",                  (u"Comum",         u"Busca",                 u"comum.busca")),
    ("followup.ui",                      (u"Comum",         u"Followup",              u"comum.followup")),
    ("sendmail.ui",                      (u"Comum",         u"E-mail",                u"comum.email")),
    ("validadedialog.ui",                (u"Comum",         u"Validade",              u"comum.validade")),
    ("inputdialog.ui",                   (u"Comum",         u"Diálogo",              u"comum.dialogo")),
    ("inputdialogconfirmacao.ui",        (u"Comum",         u"Confirmação",          u"comum.confirmacao")),
    ("inputdialogfinanceiro.ui",         (u"Comum",         u"Financeiro",            u"comum.financeiro")),
    ("inputdialogproduto.ui",            (u"Comum",         u"Produto",               u"comum.produto")),
])

# --------------------------------------------------------------------------------------------------
# Um .ui pode ser instanciado em mais de uma tela, e cada instancia tem prefixo proprio (injetado em
# runtime pelo pai via setProperty("prefixoPermissao", ...)). Sem isto, o catalogo teria as chaves de
# so uma das telas e a outra ficaria com permissao inaplicavel - ex.: Contas a Receber herdaria as
# chaves de Contas a Pagar e nao daria para configurar as duas separadamente.
INSTANCIAS_EXTRA = {
    "widgetfinanceirocontas.ui": [(u"Financeiro", u"Contas a Receber", u"financeiro.contasReceber")],
    "widgetfinanceirocompra.ui": [(u"Compras",    u"Financeiro",       u"compras.financeiro")],
    "widgetvenda.ui":            [(u"Financeiro", u"Vendas",           u"financeiro.venda")],
}

# LoginDialog roda ANTES de Permissao::carregar(); gatear seu botao trancaria o app para fora.
# collapsiblewidget e container generico (o unico botao e o proprio colapsar), nao e tela.
EXCLUIDOS = set(["logindialog.ui", "collapsiblewidget.ui", "gerenciarpermissoes.ui",
                 # contasapagar.ui e .ui orfao: nenhuma classe o usa e nem esta em FORMS no Loja.pro.
                 "contasapagar.ui"])

# Aba principal (mainwindow.ui) -> (prefixo do modulo, coluna legada de view_tab_*).
ABAS_PRINCIPAIS = OrderedDict([
    # objectName da aba -> (prefixo da chave, coluna legada, modulo de exibicao na arvore)
    ("tabOrcamentos",   (u"orcamento",    "view_tab_orcamento",  u"Orçamentos")),
    ("tabVendas",       (u"vendas",       "view_tab_venda",      u"Vendas")),
    ("tabCompras",      (u"compras",      "view_tab_compra",     u"Compras")),
    ("tabLogistica",    (u"logistica",    "view_tab_logistica",  u"Logística")),
    ("tabNFe",          (u"nfe",          "view_tab_nfe",        u"NF-e")),
    ("tabEstoque",      (u"estoque",      "view_tab_estoque",    u"Estoque")),
    ("tabGalpao",       (u"galpao",       "view_tab_galpao",     u"Galpão")),
    ("tabFinanceiro",   (u"financeiro",   "view_tab_financeiro", u"Financeiro")),
    ("tabRelatorios",   (u"relatorios",   "view_tab_relatorio",  u"Relatórios")),
    ("tabGraficos",     (u"graficos",     "view_tab_grafico",    u"Gráficos")),
    ("tabRh",           (u"rh",           "view_tab_rh",         u"RH")),
    ("tabConsistencia", (u"consistencia", None,                  u"Consistência")),  # hoje gateada por nome
])

# --------------------------------------------------------------------------------------------------
# Tipos de usuario: vem do combo do Cadastro de Usuario, nao do banco. Se viessem do banco,
# ASSISTENTE ADMINISTRATIVO e GERENTE FINANCEIRO (0 usuarios hoje, mas criaveis) nasceriam sem perfil
# e, com fail-closed, o primeiro usuario desses tipos ficaria travado.
# --------------------------------------------------------------------------------------------------
TIPOS = [
    u"ADMINISTRADOR", u"ADMINISTRATIVO", u"ASSISTENTE ADMINISTRATIVO", u"DIRETOR",
    u"GERENTE DEPARTAMENTO", u"GERENTE FINANCEIRO", u"GERENTE LOJA", u"OPERACIONAL",
    u"VENDEDOR", u"VENDEDOR ESPECIAL",
]

T_ADMIN = set([u"ADMINISTRADOR", u"DIRETOR"])                                       # User::isAdmin()
T_ADMV = set([u"ADMINISTRADOR", u"ADMINISTRATIVO", u"DIRETOR"])                     # User::isAdministrativo()
T_GERENTE = set([u"GERENTE DEPARTAMENTO", u"GERENTE FINANCEIRO", u"GERENTE LOJA"])  # User::isGerente()

# --------------------------------------------------------------------------------------------------
# INVENTARIO: chave -> conjunto de tipos que recebem 1. Todo o resto do catalogo nasce 1 para todos.
# --------------------------------------------------------------------------------------------------
INVENTARIO = {}

# mainwindow.cpp:57 - User::isAdmin()
INVENTARIO[u"menu.cadastrarUsuario"] = T_ADMIN

# mainwindow.cpp:59-71 - User::isAdministrativo()
for _chave in ["cadastrarFornecedor", "cadastrarProdutos", "gerenciarLojas", "gerenciarNCMs",
               "gerenciarTransportadoras", "gerenciarDadosBancarios", "gerenciarPagamentos",
               "gerenciarPrecoEstoque", "gerenciarStaccatoOff", "importarTabelaIBPT",
               "importarTabelaFornecedor"]:
    INVENTARIO[u"menu." + _chave] = T_ADMV

# Tela nova, so quem administra. Sem isto ninguem conseguiria abrir a tela para consertar o resto.
INVENTARIO[u"menu.gerenciarPermissoes"] = T_ADMIN

# mainwindow.cpp:96 - gateado por ajusteFrete POR USUARIO, nao e livre. Fica 0 no perfil e e liberado
# pelo override do legado, junto com sistema.ajusteFrete (ver bloco 3 do SQL).
INVENTARIO[u"menu.calcularFrete"] = set()

# mainwindow.cpp:111-115 - hoje e gate por NOME de pessoa, e um dos 3 nomes e codigo morto
# ("EDUARDO OLIVEIRA" nunca casou; o usuario real e EDUARDO PINTO DE OLIVEIRA). Vira perfil.
INVENTARIO[u"consistencia.ver"] = T_ADMIN

# Gates de botao/aba dentro de dialogos (varredura de src/*.cpp).
INVENTARIO[u"vendas.venda.cancelamento"] = T_ADMV | T_GERENTE   # venda.cpp:925 - isAdministrativo OU isGerente
INVENTARIO[u"vendas.venda.devolucao"] = T_ADMV                  # venda.cpp:927
INVENTARIO[u"vendas.venda.corrigirFluxo"] = T_ADMV              # venda.cpp:1503
INVENTARIO[u"cadastros.produto.desativar"] = T_ADMV             # cadastroproduto.cpp:25
INVENTARIO[u"cadastros.fornecedor.desativar"] = T_ADMV          # cadastrofornecedor.cpp:23
INVENTARIO[u"cadastros.fornecedor.desativarEnd"] = T_ADMV
INVENTARIO[u"cadastros.loja.desativar"] = T_ADMV                # cadastroloja.cpp:21
INVENTARIO[u"cadastros.loja.desativarEnd"] = T_ADMV
INVENTARIO[u"cadastros.transportadora.desativar"] = T_ADMV      # cadastrotransportadora.cpp:22
INVENTARIO[u"cadastros.transportadora.desativarEnd"] = T_ADMV
INVENTARIO[u"estoque.item.exibirNfe"] = T_ADMV                  # estoque.cpp:27-29
INVENTARIO[u"estoque.item.ajustarQuant"] = T_ADMV
INVENTARIO[u"sistema.config.email.ver"] = T_ADMV                # userconfig.cpp:25
INVENTARIO[u"sistema.config.nFe.ver"] = T_ADMV

# O pedido que originou o projeto: limitar movimentacao de pedido a administrador e diretoria.
INVENTARIO[u"logistica.agendarEntrega.ver"] = T_ADMIN
INVENTARIO[u"logistica.entregas.cancelarEntrega"] = T_ADMIN
for _chave in ["mover", "removerPallet", "salvarPallets", "selecionarMapa", "edicao"]:
    INVENTARIO[u"galpao.galpao." + _chave] = T_ADMIN

# Prefixo cujas chaves TODAS entram no inventario (a aba Agendar Entrega inteira, nao so o 'ver').
INVENTARIO_PREFIXO = OrderedDict([(u"logistica.agendarentrega.", T_ADMIN)])

# --------------------------------------------------------------------------------------------------
# escrita: COSMETICO (marcador na tela e filtro "so escrita"). NAO influencia o seed.
# Lista de NOME EXATO, sem curingas. Curinga e inseguro aqui: 'Consultar.*' e 'Selecionar.*'
# classificavam como leitura tres acoes que escrevem - ConsultarNFe roda 4 UPDATEs movendo
# venda_has_produto2/veiculo_has_produto para 'EM ENTREGA', e SelecionarMapa/SelecionarAssinatura
# fazem PUT no WebDAV (o primeiro sobrescreve o mapa.png global do galpao).
# --------------------------------------------------------------------------------------------------
SOMENTE_LEITURA = set([
    "Buscar", "Procurar", "Voltar", "Anterior", "Proximo", "Fechar", "Cancelar", "Selecionar",
    "Excel", "ExportarExcel", "ExportarNCM", "ExportarMes", "Exportar", "GerarExcel", "GerarPdf",
    "PDF", "Imprimir", "ImprimirDanfe", "Danfe", "AbrirDANFE", "Abrir", "AbrirOrcamento",
    "AbrirVenda", "AbrirReplicaDe", "AbrirReplicadoEm", "Relatorio", "RelatorioContabil",
    "Romaneio", "Previa", "Mapa", "Modelo3d", "Calculadora", "Calcular", "LimparFiltro",
    "LimparFiltros", "LimparSelecao", "ComprasFolder", "VendasFolder", "OrcamentosFolder",
    "EntregasPdfFolder", "EntregasXmlFolder", "Comprovantes", "ExibirNfe",
])

# Chaves sem origem em .ui: nao sao botao nem aba, vem das colunas legadas por usuario.
SISTEMA = [
    (u"sistema.webdavDocumentos", u"Rede - Documentos", "webdav_documentos"),
    (u"sistema.webdavCompras",    u"Rede - Compras",    "webdav_compras"),
    (u"sistema.webdavFinanceiro", u"Rede - Financeiro", "webdav_financeiro"),
    (u"sistema.webdavRh",         u"Rede - RH",         "webdav_rh"),
    (u"sistema.webdavObras",      u"Rede - Obras",      "webdav_obras"),
    (u"sistema.webdavLogistica",  u"Rede - Logística",  "webdav_logistica"),
    (u"sistema.ajusteFrete",      u"Ajustar Frete",     "ajusteFrete"),
]


def sufixo_da_chave(object_name):
    """Precisa bater EXATAMENTE com Permissao::sufixoDaChave (src/permissao.cpp)."""
    nome = object_name

    for prefixo in ("pushButton", "toolButton", "checkBox", "radioButton", "action", "menu", "tab"):
        if nome.startswith(prefixo) and len(nome) > len(prefixo):
            nome = nome[len(prefixo):]
            break

    while True:
        pos = nome.find("_")
        if pos == -1 or pos + 1 >= len(nome):
            break
        nome = nome[:pos] + nome[pos + 1].upper() + nome[pos + 2:]

    nome = nome.replace("_", "")

    if nome:
        nome = nome[0].lower() + nome[1:]

    return nome


def bloco_do_widget(texto, inicio):
    """Fim real de um <widget>, contando tags aninhadas.

    Regex nao-guloso para no '</widget>' do primeiro filho e classifica tabela editavel errado.
    """
    profundidade = 0
    for tag in re.finditer(r"<widget\b|</widget>", texto[inicio:]):
        profundidade += 1 if tag.group(0).startswith("<widget") else -1
        if profundidade == 0:
            return texto[inicio:inicio + tag.end()]
    return texto[inicio:]


def texto_do_widget(bloco):
    achado = re.search(r"<property name=\"text\">\s*<string>([^<]*)</string>", bloco)
    return achado.group(1).strip() if achado else u""


def escapar(valor):
    return valor.replace("\\", "\\\\").replace("'", "''")


def varrer(s, arquivo, modulo, tela, prefixo, adicionar):
    """Emite as chaves de UMA instancia do .ui (modulo/tela/prefixo)."""
    ordem = 0

    # --- abas -----------------------------------------------------------------------------
    # Aba de titulo vazio (<string/>) fica de fora de proposito: orcamento.ui/tabNovoOrc e
    # venda.ui/tab sao conteineres de aba unica, sem titulo visivel. Gatea-las esconderia a tela
    # inteira sem o usuario entender por que.
    for m in re.finditer(r"<widget class=\"QWidget\" name=\"(\w+)\">\s*<attribute name=\"title\">\s*<string>([^<]+)</string>", s):
        ordem += 1
        nome, titulo = m.group(1), m.group(2).strip()

        if arquivo == "mainwindow.ui":
            # Aba principal: a chave e do MODULO, nao 'menu.*'.
            if nome not in ABAS_PRINCIPAIS:
                continue
            # Módulo de exibição próprio, não o do mainwindow.ui: senão "Ver Logística" apareceria
            # sob o nó Menu na árvore da tela, em vez de sob Logística.
            prefixo_mod, _coluna_legada, modulo_exibicao = ABAS_PRINCIPAIS[nome]
            adicionar(prefixo_mod + u".ver", modulo_exibicao, u"Acesso", u"Ver " + titulo,
                      u"ver", 0, ordem, arquivo)
            continue

        adicionar(u"%s.%s.ver" % (prefixo, sufixo_da_chave(nome)), modulo, tela,
                  u"Ver " + titulo, u"ver", 0, ordem, arquivo)

    # --- botoes ---------------------------------------------------------------------------
    for m in re.finditer(r"<widget class=\"(QPushButton|QToolButton|QCheckBox|QRadioButton)\" name=\"(\w+)\"", s):
        ordem += 1
        classe, nome = m.group(1), m.group(2)
        chave = u"%s.%s" % (prefixo, sufixo_da_chave(nome))

        # QCheckBox/QRadioButton so entram quando ha gate real hoje - senao os checkboxes/radios de
        # filtro (MostrarInativos, filtros de status etc.) viram permissao a toa. Chave ausente =
        # permitido, entao os que ficam de fora seguem funcionando normalmente.
        if classe in ("QCheckBox", "QRadioButton") and chave not in INVENTARIO:
            continue

        base = re.sub(r"^(pushButton|toolButton|checkBox|radioButton)", "", nome)
        adicionar(chave, modulo, tela, texto_do_widget(bloco_do_widget(s, m.start())),
                  u"acao", 0 if base in SOMENTE_LEITURA else 1, ordem, arquivo)

    # --- tabelas editaveis (uma chave por tela, nao por tabela) ----------------------------
    for m in re.finditer(r"<widget class=\"(TableView|QTableView)\" name=\"(\w+)\"", s):
        if "NoEditTriggers" in bloco_do_widget(s, m.start()):
            continue
        ordem += 1
        adicionar(prefixo + u".editarTabela", modulo, tela, u"Editar a tabela", u"tabela", 1,
                  ordem, arquivo)
        break

    # --- acoes de menu --------------------------------------------------------------------
    if arquivo == "mainwindow.ui":
        for m in re.finditer(r"<action name=\"(\w+)\">(.*?)</action>", s, re.S):
            ordem += 1
            titulo = re.search(r"<string>([^<]*)</string>", m.group(2))
            adicionar(u"menu." + sufixo_da_chave(m.group(1)), u"Menu", u"Menu",
                      titulo.group(1).strip() if titulo else m.group(1), u"acao", 1, ordem, arquivo)

        # O QMenu 'Importar tabela fornecedor' e gateado pela propria menuAction()
        # (mainwindow.cpp:70) - nao aparece como <action>.
        for m in re.finditer(r"<widget class=\"QMenu\" name=\"(menuImportar_tabela_fornecedor)\">", s):
            ordem += 1
            adicionar(u"menu." + sufixo_da_chave(m.group(1)), u"Menu", u"Menu",
                      u"Importar tabela fornecedor", u"acao", 1, ordem, arquivo)

        # menu.gerenciarPermissoes ja sai da varredura de <action> acima: actionGerenciar_Permissoes
        # existe em mainwindow.ui.


def varrer_salvar(adicionar, prefixo_para_modulo_tela):
    """Chave '<prefixo>.salvar': o guarda autoritativo de RegisterDialog::save() (registerdialog.cpp)
    checa sempre '<prefixoPermissao>.salvar', mas nenhuma tela do projeto tem um pushButtonSalvar de
    verdade (o padrao real e pushButtonCadastrar/pushButtonAtualizar/etc) - entao essa chave nunca
    nasceria da varredura de botoes do .ui. Sem esta funcao a chave fica ausente do catalogo e o
    guarda vira no-op (chave ausente = permitido) para toda tela que usa prefixoPermissao.

    Levantado do fonte, nao mantido a mao: varre src/*.cpp por 'prefixoPermissao = "..."'.
    """
    padrao = re.compile(r'prefixoPermissao\s*=\s*"([^"]+)"')

    for caminho in sorted(glob.glob(os.path.join(RAIZ, "src", "*.cpp"))):
        with io.open(caminho, encoding="utf-8") as fh:
            texto = fh.read()

        for m in padrao.finditer(texto):
            prefixo = m.group(1)

            if prefixo not in prefixo_para_modulo_tela:
                raise SystemExit(u"%s define prefixoPermissao '%s' sem entrada correspondente no MAPA."
                                  % (os.path.basename(caminho), prefixo))

            modulo, tela = prefixo_para_modulo_tela[prefixo]
            adicionar(prefixo + u".salvar", modulo, tela, u"Salvar", u"acao", 1, 0, os.path.basename(caminho))


def coletar():
    """Devolve [(chave, modulo, tela, descricao, tipoAcao, escrita, ordem)] e os conflitos achados."""
    permissoes = []
    origem = {}
    conflitos = []

    def adicionar(chave, modulo, tela, descricao, tipo_acao, escrita, ordem, arquivo):
        if chave in origem:
            conflitos.append((chave, origem[chave], arquivo))
            return
        origem[chave] = arquivo
        permissoes.append((chave, modulo, tela, descricao or chave.split(".")[-1], tipo_acao, escrita, ordem))

    for arquivo in sorted(os.listdir(UI)):
        if not arquivo.endswith(".ui") or arquivo in EXCLUIDOS:
            continue

        if arquivo not in MAPA:
            raise SystemExit(u"ui/%s nao esta no MAPA - adicione antes de gerar o seed." % arquivo)

        with io.open(os.path.join(UI, arquivo), encoding="utf-8") as fh:
            s = fh.read()

        for modulo, tela, prefixo in [MAPA[arquivo]] + INSTANCIAS_EXTRA.get(arquivo, []):
            varrer(s, arquivo, modulo, tela, prefixo, adicionar)

    # --- '<prefixo>.salvar', levantado de src/*.cpp (ver varrer_salvar) -----------------------
    prefixo_para_modulo_tela = {}
    for modulo, tela, prefixo in list(MAPA.values()) + [item for lista in INSTANCIAS_EXTRA.values() for item in lista]:
        prefixo_para_modulo_tela[prefixo] = (modulo, tela)

    varrer_salvar(adicionar, prefixo_para_modulo_tela)

    # --- chaves sem origem em .ui -------------------------------------------------------------
    for i, (chave, descricao, _coluna) in enumerate(SISTEMA):
        adicionar(chave, u"Sistema", u"Sistema", descricao, u"acao", 1, i + 1, u"(legado)")

    return permissoes, conflitos


def tipos_permitidos(chave):
    """None quando a chave nao esta no inventario (= permitida para todos)."""
    for prefixo, tipos in INVENTARIO_PREFIXO.items():
        if chave.lower().startswith(prefixo):
            return tipos
    return INVENTARIO.get(chave)


def main():
    incluir_paridade = "--com-paridade-legado" in sys.argv

    permissoes, conflitos = coletar()

    if conflitos:
        print(u"ERRO: chaves duplicadas (dois .ui gerando a mesma chave):")
        for chave, primeiro, segundo in conflitos:
            print(u"  %-55s %s x %s" % (chave, primeiro, segundo))
        raise SystemExit(1)

    # Toda chave do inventario tem que existir no catalogo, senao o bloqueio nao tem efeito.
    chaves = set(c for c, _m, _t, _d, _a, _e, _o in permissoes)
    orfas = sorted(k for k in INVENTARIO if k not in chaves)
    if orfas:
        print(u"ERRO: chaves do INVENTARIO que nao existem no catalogo (bloqueio sem efeito):")
        for k in orfas:
            print(u"  " + k)
        raise SystemExit(1)

    linhas = []
    w = linhas.append

    w(u"-- GERADO POR db/gerar_catalogo_permissoes.py - NAO EDITAR A MAO.")
    w(u"-- Reexecute o script depois de mexer nos .ui; blocos 1 e 2 sao insert-only e preservam o que")
    w(u"-- ja foi configurado em producao via Gerenciar Permissoes.")
    w(u"--")
    if incluir_paridade:
        w(u"-- Gerado com --com-paridade-legado: inclui o bloco 3 (backfill de usuario_has_permissao).")
        w(u"-- Rodar SO no corte inicial - reexecutar depois disso reverteria ajustes feitos em producao")
        w(u"-- nas 19 chaves legadas. Para regenerar o catalogo no dia a dia, rodar SEM essa flag.")
    else:
        w(u"-- Gerado SEM --com-paridade-legado: bloco 3 (backfill de usuario_has_permissao) omitido.")
        w(u"-- Modo de uso rotineiro, seguro para reexecutar apos qualquer mudanca em .ui.")
    w(u"--")
    w(u"-- Pre-requisito: db/permissoes_v2.sql aplicado (bloco APPLY descomentado).")
    w(u"-- Depois deste script, rodar as CONFERENCIAS de db/permissoes_v2.sql ANTES de publicar o build.")
    w(u"")
    w(u"START TRANSACTION;")
    w(u"")
    w(u"-- ================= 1. CATALOGO (%d chaves) =================" % len(permissoes))
    w(u"INSERT INTO permissao (chave, modulo, tela, descricao, tipoAcao, escrita, ordem) VALUES")

    valores = []
    for chave, modulo, tela, descricao, tipo_acao, escrita, ordem in permissoes:
        valores.append(u"  ('%s', '%s', '%s', '%s', '%s', %d, %d)" % (
            escapar(chave), escapar(modulo), escapar(tela), escapar(descricao[:120]),
            tipo_acao, escrita, ordem))
    w(u",\n".join(valores))
    w(u"AS novo ON DUPLICATE KEY UPDATE modulo = novo.modulo, tela = novo.tela,")
    w(u"  descricao = novo.descricao, tipoAcao = novo.tipoAcao, escrita = novo.escrita, ordem = novo.ordem;")
    w(u"")

    w(u"-- ================= 2. PERFIS (%d tipos x %d chaves) =================" % (len(TIPOS), len(permissoes)))
    w(u"-- Default 1. Recebe 0 so onde ha bloqueio hoje (inventario levantado do fonte).")
    # Tabela derivada aliasada: INSERT ... SELECT nao aceita o row alias "AS novo".
    w(u"INSERT INTO perfil_has_permissao (tipo, idPermissao, permitido)")
    w(u"SELECT novo.tipo, novo.idPermissao, novo.permitido FROM (")
    w(u"  SELECT t.tipo AS tipo, p.idPermissao AS idPermissao, CASE")

    por_conjunto = OrderedDict()
    for chave, _m, _t, _d, _a, _e, _o in permissoes:
        tipos = tipos_permitidos(chave)
        if tipos is None:
            continue
        por_conjunto.setdefault(frozenset(tipos), []).append(chave)

    for tipos, lista in por_conjunto.items():
        lista_chaves = u", ".join(u"'%s'" % escapar(c) for c in sorted(lista))
        if tipos:
            lista_tipos = u", ".join(u"'%s'" % escapar(t) for t in sorted(tipos))
            w(u"      WHEN p.chave IN (%s) THEN IF(t.tipo IN (%s), 1, 0)" % (lista_chaves, lista_tipos))
        else:
            w(u"      WHEN p.chave IN (%s) THEN 0" % lista_chaves)

    w(u"    ELSE 1 END AS permitido")
    w(u"  FROM permissao p")
    w(u"  CROSS JOIN (SELECT %s) t" % u" UNION ALL SELECT ".join(u"'%s' AS tipo" % escapar(t) for t in TIPOS))
    # Insert-only: nao sobrescreve permitido de linha ja existente, senao reexecutar o script depois
    # de mudanca de configuracao em producao (via Gerenciar Permissoes) reverteria o que foi mudado.
    # Qualificado com o nome da tabela: 'permitido = permitido' sem qualificar da erro 1052 ambiguous
    # column no MySQL 8.4 (colide com o novo.permitido da subquery derivada).
    w(u") AS novo ON DUPLICATE KEY UPDATE permitido = perfil_has_permissao.permitido;")
    w(u"")

    # Bloco 3: SO no corte inicial (--com-paridade-legado). Le de usuario_has_permissao (congelada) e
    # SOBRESCREVE pf.permitido/override incondicionalmente - nao e insert-only feito de proposito,
    # porque aqui a fonte de verdade e a tabela legada, nao o que ja esta em perfil_has_permissao.
    # Reexecutar isso depois do corte reverteria qualquer ajuste feito em producao nessas 19 chaves.
    if incluir_paridade:
        w(u"-- ================= 3. PARIDADE DAS CHAVES LEGADAS =================")
        w(u"-- As 19 permissoes antigas eram POR USUARIO, e nao ha padrao por tipo: 77 combinacoes")
        w(u"-- distintas entre os ativos, 36 so entre os 58 ADMINISTRATIVO. Perfil = maioria do tipo;")
        w(u"-- quem diverge ganha override. Assim ninguem perde nem ganha acesso no dia 1.")
        w(u"")

        legado = []
        for nome, (mod_prefixo, coluna, _modulo) in ABAS_PRINCIPAIS.items():
            if coluna:
                legado.append((mod_prefixo + u".ver", coluna))
        for chave, _descricao, coluna in SISTEMA:
            legado.append((chave, coluna))

        for chave, coluna in legado:
            w(u"-- %s  <-  usuario_has_permissao.%s" % (chave, coluna))
            w(u"UPDATE perfil_has_permissao pf JOIN permissao pe ON pe.idPermissao = pf.idPermissao")
            w(u"SET pf.permitido = COALESCE((")
            w(u"  SELECT IF(AVG(COALESCE(a.%s, 0)) >= 0.5, 1, 0) FROM usuario u" % coluna)
            w(u"    JOIN usuario_has_permissao a ON a.idUsuario = u.idUsuario")
            w(u"    WHERE u.desativado = 0 AND u.tipo = pf.tipo), 0)")
            w(u"WHERE pe.chave = '%s';" % escapar(chave))
            w(u"")
            w(u"INSERT INTO usuario_has_permissao_override (idUsuario, idPermissao, permitido)")
            w(u"SELECT novo.idUsuario, novo.idPermissao, novo.permitido FROM (")
            w(u"  SELECT u.idUsuario AS idUsuario, pe.idPermissao AS idPermissao, COALESCE(a.%s, 0) AS permitido" % coluna)
            w(u"  FROM usuario u")
            w(u"    JOIN usuario_has_permissao a ON a.idUsuario = u.idUsuario")
            w(u"    JOIN permissao pe ON pe.chave = '%s'" % escapar(chave))
            w(u"    JOIN perfil_has_permissao pf ON pf.idPermissao = pe.idPermissao AND pf.tipo = u.tipo")
            w(u"  WHERE u.desativado = 0 AND COALESCE(a.%s, 0) <> pf.permitido" % coluna)
            w(u") AS novo ON DUPLICATE KEY UPDATE permitido = novo.permitido;")
            w(u"")

        w(u"-- menu.calcularFrete acompanha sistema.ajusteFrete (mainwindow.cpp:96 gateia por usuario).")
        w(u"INSERT INTO usuario_has_permissao_override (idUsuario, idPermissao, permitido)")
        w(u"SELECT novo.idUsuario, novo.idPermissao, novo.permitido FROM (")
        w(u"  SELECT o.idUsuario AS idUsuario, alvo.idPermissao AS idPermissao, o.permitido AS permitido")
        w(u"  FROM usuario_has_permissao_override o")
        w(u"    JOIN permissao origem ON origem.idPermissao = o.idPermissao AND origem.chave = 'sistema.ajusteFrete'")
        w(u"    JOIN permissao alvo ON alvo.chave = 'menu.calcularFrete'")
        w(u") AS novo ON DUPLICATE KEY UPDATE permitido = novo.permitido;")
        w(u"")

    # Sempre roda, com ou sem --com-paridade-legado: so ZERA o filho quando o pai ja esta em 0 - nunca
    # concede nem desfaz um bloqueio mais fino que um admin tenha posto so no filho. Diferente do bloco
    # 3, nao ha risco de reverter configuracao feita em Gerenciar Permissoes.
    w(u"-- Sub-abas herdam a aba principal: sem ver o modulo, nao ve as telas dele.")
    # Auto-join, nao subconsulta: o MySQL recusa (erro 1093) ler a tabela que esta sendo atualizada
    # dentro de um EXISTS. As chaves de aba principal ('logistica.ver') tem um ponto so, entao nao
    # sao alvo deste UPDATE e nao ha interferencia entre o que se le e o que se escreve.
    w(u"UPDATE perfil_has_permissao pf")
    w(u"  JOIN permissao pe ON pe.idPermissao = pf.idPermissao")
    w(u"  JOIN permissao pe2 ON pe2.chave = CONCAT(SUBSTRING_INDEX(pe.chave, '.', 1), '.ver')")
    w(u"  JOIN perfil_has_permissao pf2 ON pf2.idPermissao = pe2.idPermissao AND pf2.tipo = pf.tipo")
    w(u"SET pf.permitido = 0")
    w(u"WHERE pe.tipoAcao = 'ver' AND pe.chave LIKE '%.%.ver' AND pf2.permitido = 0;")
    w(u"")

    w(u"COMMIT;")
    w(u"")

    with io.open(SAIDA, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(u"\n".join(linhas))

    por_acao = {}
    escritas = 0
    for _c, _m, _t, _d, acao, escrita, _o in permissoes:
        por_acao[acao] = por_acao.get(acao, 0) + 1
        escritas += escrita

    print(u"%s gerado" % os.path.relpath(SAIDA, RAIZ))
    print(u"  chaves        : %d" % len(permissoes))
    for acao in sorted(por_acao):
        print(u"    %-8s: %d" % (acao, por_acao[acao]))
    print(u"  escrita=1     : %d" % escritas)
    print(u"  perfis        : %d tipos x %d = %d linhas" % (len(TIPOS), len(permissoes), len(TIPOS) * len(permissoes)))
    print(u"  no inventario : %d chaves com bloqueio" % len(INVENTARIO))
    print(u"  paridade legado: %s" % (u"incluida (--com-paridade-legado)" if incluir_paridade else u"omitida (modo rotina)"))


if __name__ == "__main__":
    main()
