#-------------------------------------------------
# Static lib containing all production sources (src/* except main.cpp)
# Loja.pro and tests/*.pro both link against this lib.
# See .claude/test-infrastructure-plan.md (M1) for rationale.
#-------------------------------------------------

!versionAtLeast(QT_VERSION, 5.15.0) {
    error("Use Qt 5.15 ou mais novo")
}

TARGET = staccato
TEMPLATE = lib
CONFIG  += staticlib

ROOT_PWD = $$PWD/..

include($$ROOT_PWD/3rdparty/QtXlsxWriter/src/xlsx/qtxlsx.pri)
include($$ROOT_PWD/3rdparty/QSimpleUpdater/qsimpleupdater.pri)
# LimeReport's .pri adds lrfactoryinitializer.cpp under `staticlib`, but that
# file transitively needs the designer.pri sources (which the project doesn't
# enable). Temporarily drop the flag around the include so the .pri doesn't
# add lrfactoryinitializer.cpp — we get it back below.
CONFIG -= staticlib
versionAtMost(QT_VERSION, 5.15.2) { include($$ROOT_PWD/3rdparty/LimeReport-1.5.68/limereport/limereport.pri) }
CONFIG += staticlib

# RESOURCES from the 3rdparty .pri files (QSimpleUpdater's qsu_resources.qrc,
# LimeReport's report.qrc and items.qrc) are intentionally dropped here so the
# qrc_*.cpp global ctors live in Loja.exe (and other exes) instead of the
# static lib. Static-lib resources on MSVC require Q_INIT_RESOURCE() calls
# from main(); putting them in the exe avoids that requirement.
RESOURCES =

QT *= core gui sql network xml charts widgets

DEFINES *= QT_DEPRECATED_WARNINGS

CONFIG *= c++latest warn_on

PRECOMPILED_HEADER = $$ROOT_PWD/pch.h
CONFIG *= precompile_header

*-g++ {
    QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
    QMAKE_CXXFLAGS *= -Wall -Wextra -Wpedantic
}

*-clang {
    QMAKE_CXXFLAGS *= -Wno-deprecated-enum-enum-conversion
}

win32-msvc {
    QMAKE_CXXFLAGS += /permissive-
}

win32-g++ {
    exists($(QTDIR)/bin/ccache.exe) {
        message("using ccache")
        QMAKE_CC = ccache $$QMAKE_CC
        QMAKE_CXX = ccache $$QMAKE_CXX
        QMAKE_CXXFLAGS += -fpch-preprocess
    }
}

linux {
    CCACHE_BIN = $$system(which ccache)

    !isEmpty(CCACHE_BIN) {
        message("using ccache")
        QMAKE_CC = ccache $$QMAKE_CC
        QMAKE_CXX = ccache $$QMAKE_CXX
        QMAKE_CXXFLAGS += -fpch-preprocess
    }
}

#-----------------------------------------------------

MOC_DIR        = build_files/moc
UI_DIR         = build_files/ui
UI_HEADERS_DIR = build_files/ui
UI_SOURCES_DIR = build_files/ui
OBJECTS_DIR    = build_files/obj
RCC_DIR        = build_files/rcc

#-----------------------------------------------------

# Repo root must be on INCLUDEPATH because the .ui files reference custom
# widgets via paths like <header>src/itembox.h</header>; UIC emits the path
# verbatim, so `#include "src/itembox.h"` only resolves when the repo root
# is searched. (In the legacy single-pro layout, cl was invoked from the repo
# root and `-I.` covered this.)
INCLUDEPATH += $$ROOT_PWD $$ROOT_PWD/src

SOURCES += \
    $$ROOT_PWD/src/QtCUrl.cpp \
    $$ROOT_PWD/src/acbr.cpp \
    $$ROOT_PWD/src/acbrlib.cpp \
    $$ROOT_PWD/src/anteciparrecebimento.cpp \
    $$ROOT_PWD/src/application.cpp \
    $$ROOT_PWD/src/baixaorcamento.cpp \
    $$ROOT_PWD/src/cadastrarnfe.cpp \
    $$ROOT_PWD/src/cadastrocliente.cpp \
    $$ROOT_PWD/src/cadastrofornecedor.cpp \
    $$ROOT_PWD/src/cadastrofuncionario.cpp \
    $$ROOT_PWD/src/cadastroloja.cpp \
    $$ROOT_PWD/src/cadastroncm.cpp \
    $$ROOT_PWD/src/cadastropagamento.cpp \
    $$ROOT_PWD/src/cadastroproduto.cpp \
    $$ROOT_PWD/src/cadastroprofissional.cpp \
    $$ROOT_PWD/src/cadastrostaccatooff.cpp \
    $$ROOT_PWD/src/cadastrotransportadora.cpp \
    $$ROOT_PWD/src/cadastrousuario.cpp \
    $$ROOT_PWD/src/calculofrete.cpp \
    $$ROOT_PWD/src/cancelaproduto.cpp \
    $$ROOT_PWD/src/cepcompleter.cpp \
    $$ROOT_PWD/src/charttooltip.cpp \
    $$ROOT_PWD/src/chartview.cpp \
    $$ROOT_PWD/src/checkboxdelegate.cpp \
    $$ROOT_PWD/src/cnab.cpp \
    $$ROOT_PWD/src/collapsiblewidget.cpp \
    $$ROOT_PWD/src/comboboxdelegate.cpp \
    $$ROOT_PWD/src/compraavulsa.cpp \
    $$ROOT_PWD/src/comprovantes.cpp \
    $$ROOT_PWD/src/contas.cpp \
    $$ROOT_PWD/src/dateformatdelegate.cpp \
    $$ROOT_PWD/src/devolucao.cpp \
    $$ROOT_PWD/src/doubledelegate.cpp \
    $$ROOT_PWD/src/doublespinbox.cpp \
    $$ROOT_PWD/src/editdelegate.cpp \
    $$ROOT_PWD/src/estoque.cpp \
    $$ROOT_PWD/src/estoqueitem.cpp \
    $$ROOT_PWD/src/estoqueprazoproxymodel.cpp \
    $$ROOT_PWD/src/estoqueproxymodel.cpp \
    $$ROOT_PWD/src/excel.cpp \
    $$ROOT_PWD/src/file.cpp \
    $$ROOT_PWD/src/financeiroproxymodel.cpp \
    $$ROOT_PWD/src/followup.cpp \
    $$ROOT_PWD/src/followupproxymodel.cpp \
    $$ROOT_PWD/src/importaprodutos.cpp \
    $$ROOT_PWD/src/importaprodutosproxymodel.cpp \
    $$ROOT_PWD/src/importarxml.cpp \
    $$ROOT_PWD/src/importatabelaibpt.cpp \
    $$ROOT_PWD/src/inputdialog.cpp \
    $$ROOT_PWD/src/inputdialogconfirmacao.cpp \
    $$ROOT_PWD/src/inputdialogfinanceiro.cpp \
    $$ROOT_PWD/src/inputdialogproduto.cpp \
    $$ROOT_PWD/src/inserirlancamento.cpp \
    $$ROOT_PWD/src/inserirtransferencia.cpp \
    $$ROOT_PWD/src/itembox.cpp \
    $$ROOT_PWD/src/itemboxdelegate.cpp \
    $$ROOT_PWD/src/lineedit.cpp \
    $$ROOT_PWD/src/lineeditcep.cpp \
    $$ROOT_PWD/src/lineeditdelegate.cpp \
    $$ROOT_PWD/src/lineedittel.cpp \
    $$ROOT_PWD/src/log.cpp \
    $$ROOT_PWD/src/logindialog.cpp \
    $$ROOT_PWD/src/mainwindow.cpp \
    $$ROOT_PWD/src/nfeproxymodel.cpp \
    $$ROOT_PWD/src/noeditdelegate.cpp \
    $$ROOT_PWD/src/orcamento.cpp \
    $$ROOT_PWD/src/orcamentoproxymodel.cpp \
    $$ROOT_PWD/src/pagamentosdia.cpp \
    $$ROOT_PWD/src/palletitem.cpp \
    $$ROOT_PWD/src/pdf.cpp \
    $$ROOT_PWD/src/porcentagemdelegate.cpp \
    $$ROOT_PWD/src/precoestoque.cpp \
    $$ROOT_PWD/src/produtoproxymodel.cpp \
    $$ROOT_PWD/src/produtospendentes.cpp \
    $$ROOT_PWD/src/reaisdelegate.cpp \
    $$ROOT_PWD/src/registeraddressdialog.cpp \
    $$ROOT_PWD/src/registerdialog.cpp \
    $$ROOT_PWD/src/scrollarea.cpp \
    $$ROOT_PWD/src/searchdialog.cpp \
    $$ROOT_PWD/src/sendmail.cpp \
    $$ROOT_PWD/src/smtp.cpp \
    $$ROOT_PWD/src/sortfilterproxymodel.cpp \
    $$ROOT_PWD/src/sql.cpp \
    $$ROOT_PWD/src/sqlquery.cpp \
    $$ROOT_PWD/src/sqlquerymodel.cpp \
    $$ROOT_PWD/src/sqltablemodel.cpp \
    $$ROOT_PWD/src/sqltreemodel.cpp \
    $$ROOT_PWD/src/tabcompras.cpp \
    $$ROOT_PWD/src/tabestoque.cpp \
    $$ROOT_PWD/src/tabfinanceiro.cpp \
    $$ROOT_PWD/src/tabgalpao.cpp \
    $$ROOT_PWD/src/tableview.cpp \
    $$ROOT_PWD/src/tablogistica.cpp \
    $$ROOT_PWD/src/tabnfe.cpp \
    $$ROOT_PWD/src/treeview.cpp \
    $$ROOT_PWD/src/user.cpp \
    $$ROOT_PWD/src/userconfig.cpp \
    $$ROOT_PWD/src/validadedialog.cpp \
    $$ROOT_PWD/src/validators.cpp \
    $$ROOT_PWD/src/venda.cpp \
    $$ROOT_PWD/src/vendaproxymodel.cpp \
    $$ROOT_PWD/src/viewgalpao.cpp \
    $$ROOT_PWD/src/widgetcompraavulsa.cpp \
    $$ROOT_PWD/src/widgetcompraconfirmar.cpp \
    $$ROOT_PWD/src/widgetcompraconsumos.cpp \
    $$ROOT_PWD/src/widgetcompradevolucao.cpp \
    $$ROOT_PWD/src/widgetcomprafaturar.cpp \
    $$ROOT_PWD/src/widgetcompragerar.cpp \
    $$ROOT_PWD/src/widgetcomprahistorico.cpp \
    $$ROOT_PWD/src/widgetcomprapendentes.cpp \
    $$ROOT_PWD/src/widgetcompraresumo.cpp \
    $$ROOT_PWD/src/widgetconsistencia.cpp \
    $$ROOT_PWD/src/widgetdevolucao.cpp \
    $$ROOT_PWD/src/widgetestoqueproduto.cpp \
    $$ROOT_PWD/src/widgetestoques.cpp \
    $$ROOT_PWD/src/widgetfinanceirocompra.cpp \
    $$ROOT_PWD/src/widgetfinanceirocontas.cpp \
    $$ROOT_PWD/src/widgetfinanceirofluxocaixa.cpp \
    $$ROOT_PWD/src/widgetgalpao.cpp \
    $$ROOT_PWD/src/widgetgalpaopeso.cpp \
    $$ROOT_PWD/src/widgetgare.cpp \
    $$ROOT_PWD/src/widgetgraficos.cpp \
    $$ROOT_PWD/src/widgetlogisticaagendarcoleta.cpp \
    $$ROOT_PWD/src/widgetlogisticaagendarentrega.cpp \
    $$ROOT_PWD/src/widgetlogisticacalendario.cpp \
    $$ROOT_PWD/src/widgetlogisticacaminhao.cpp \
    $$ROOT_PWD/src/widgetlogisticacoleta.cpp \
    $$ROOT_PWD/src/widgetlogisticaentregas.cpp \
    $$ROOT_PWD/src/widgetlogisticaentregues.cpp \
    $$ROOT_PWD/src/widgetlogisticarecebimento.cpp \
    $$ROOT_PWD/src/widgetlogisticarepresentacao.cpp \
    $$ROOT_PWD/src/widgetnfedistribuicao.cpp \
    $$ROOT_PWD/src/widgetnfeentrada.cpp \
    $$ROOT_PWD/src/widgetnfesaida.cpp \
    $$ROOT_PWD/src/widgetorcamento.cpp \
    $$ROOT_PWD/src/widgetpagamentos.cpp \
    $$ROOT_PWD/src/widgetrelatorio.cpp \
    $$ROOT_PWD/src/widgetrh.cpp \
    $$ROOT_PWD/src/widgetvenda.cpp \
    $$ROOT_PWD/src/xml.cpp \
    $$ROOT_PWD/src/xml_viewer.cpp

HEADERS += \
    $$ROOT_PWD/src/QtCUrl.h \
    $$ROOT_PWD/src/acbr.h \
    $$ROOT_PWD/src/acbrlib.h \
    $$ROOT_PWD/src/anteciparrecebimento.h \
    $$ROOT_PWD/src/application.h \
    $$ROOT_PWD/src/baixaorcamento.h \
    $$ROOT_PWD/src/cadastrarnfe.h \
    $$ROOT_PWD/src/cadastrocliente.h \
    $$ROOT_PWD/src/cadastrofornecedor.h \
    $$ROOT_PWD/src/cadastrofuncionario.h \
    $$ROOT_PWD/src/cadastroloja.h \
    $$ROOT_PWD/src/cadastroncm.h \
    $$ROOT_PWD/src/cadastropagamento.h \
    $$ROOT_PWD/src/cadastroproduto.h \
    $$ROOT_PWD/src/cadastroprofissional.h \
    $$ROOT_PWD/src/cadastrostaccatooff.h \
    $$ROOT_PWD/src/cadastrotransportadora.h \
    $$ROOT_PWD/src/cadastrousuario.h \
    $$ROOT_PWD/src/calculofrete.h \
    $$ROOT_PWD/src/cancelaproduto.h \
    $$ROOT_PWD/src/cepcompleter.h \
    $$ROOT_PWD/src/charttooltip.h \
    $$ROOT_PWD/src/chartview.h \
    $$ROOT_PWD/src/checkboxdelegate.h \
    $$ROOT_PWD/src/cnab.h \
    $$ROOT_PWD/src/collapsiblewidget.h \
    $$ROOT_PWD/src/comboboxdelegate.h \
    $$ROOT_PWD/src/compraavulsa.h \
    $$ROOT_PWD/src/comprovantes.h \
    $$ROOT_PWD/src/contas.h \
    $$ROOT_PWD/src/dateformatdelegate.h \
    $$ROOT_PWD/src/devolucao.h \
    $$ROOT_PWD/src/doubledelegate.h \
    $$ROOT_PWD/src/doublespinbox.h \
    $$ROOT_PWD/src/editdelegate.h \
    $$ROOT_PWD/src/estoque.h \
    $$ROOT_PWD/src/estoqueitem.h \
    $$ROOT_PWD/src/estoqueprazoproxymodel.h \
    $$ROOT_PWD/src/estoqueproxymodel.h \
    $$ROOT_PWD/src/excel.h \
    $$ROOT_PWD/src/file.h \
    $$ROOT_PWD/src/financeiroproxymodel.h \
    $$ROOT_PWD/src/followup.h \
    $$ROOT_PWD/src/followupproxymodel.h \
    $$ROOT_PWD/src/importaprodutos.h \
    $$ROOT_PWD/src/importaprodutosproxymodel.h \
    $$ROOT_PWD/src/importarxml.h \
    $$ROOT_PWD/src/importatabelaibpt.h \
    $$ROOT_PWD/src/inputdialog.h \
    $$ROOT_PWD/src/inputdialogconfirmacao.h \
    $$ROOT_PWD/src/inputdialogfinanceiro.h \
    $$ROOT_PWD/src/inputdialogproduto.h \
    $$ROOT_PWD/src/inserirlancamento.h \
    $$ROOT_PWD/src/inserirtransferencia.h \
    $$ROOT_PWD/src/itembox.h \
    $$ROOT_PWD/src/itemboxdelegate.h \
    $$ROOT_PWD/src/lineedit.h \
    $$ROOT_PWD/src/lineeditcep.h \
    $$ROOT_PWD/src/lineeditdelegate.h \
    $$ROOT_PWD/src/lineedittel.h \
    $$ROOT_PWD/src/log.h \
    $$ROOT_PWD/src/logindialog.h \
    $$ROOT_PWD/src/mainwindow.h \
    $$ROOT_PWD/src/nfeproxymodel.h \
    $$ROOT_PWD/src/noeditdelegate.h \
    $$ROOT_PWD/src/orcamento.h \
    $$ROOT_PWD/src/orcamentoproxymodel.h \
    $$ROOT_PWD/src/pagamentosdia.h \
    $$ROOT_PWD/src/palletitem.h \
    $$ROOT_PWD/src/pdf.h \
    $$ROOT_PWD/src/porcentagemdelegate.h \
    $$ROOT_PWD/src/precoestoque.h \
    $$ROOT_PWD/src/produtoproxymodel.h \
    $$ROOT_PWD/src/produtospendentes.h \
    $$ROOT_PWD/src/reaisdelegate.h \
    $$ROOT_PWD/src/registeraddressdialog.h \
    $$ROOT_PWD/src/registerdialog.h \
    $$ROOT_PWD/src/scrollarea.h \
    $$ROOT_PWD/src/searchdialog.h \
    $$ROOT_PWD/src/sendmail.h \
    $$ROOT_PWD/src/smtp.h \
    $$ROOT_PWD/src/sortfilterproxymodel.h \
    $$ROOT_PWD/src/sql.h \
    $$ROOT_PWD/src/sqlquery.h \
    $$ROOT_PWD/src/sqlquerymodel.h \
    $$ROOT_PWD/src/sqltablemodel.h \
    $$ROOT_PWD/src/sqltreemodel.h \
    $$ROOT_PWD/src/tabcompras.h \
    $$ROOT_PWD/src/tabestoque.h \
    $$ROOT_PWD/src/tabfinanceiro.h \
    $$ROOT_PWD/src/tabgalpao.h \
    $$ROOT_PWD/src/tableview.h \
    $$ROOT_PWD/src/tablogistica.h \
    $$ROOT_PWD/src/tabnfe.h \
    $$ROOT_PWD/src/treeview.h \
    $$ROOT_PWD/src/user.h \
    $$ROOT_PWD/src/userconfig.h \
    $$ROOT_PWD/src/validadedialog.h \
    $$ROOT_PWD/src/validators.h \
    $$ROOT_PWD/src/venda.h \
    $$ROOT_PWD/src/vendaproxymodel.h \
    $$ROOT_PWD/src/viewgalpao.h \
    $$ROOT_PWD/src/widgetcompraavulsa.h \
    $$ROOT_PWD/src/widgetcompraconfirmar.h \
    $$ROOT_PWD/src/widgetcompraconsumos.h \
    $$ROOT_PWD/src/widgetcompradevolucao.h \
    $$ROOT_PWD/src/widgetcomprafaturar.h \
    $$ROOT_PWD/src/widgetcompragerar.h \
    $$ROOT_PWD/src/widgetcomprahistorico.h \
    $$ROOT_PWD/src/widgetcomprapendentes.h \
    $$ROOT_PWD/src/widgetcompraresumo.h \
    $$ROOT_PWD/src/widgetconsistencia.h \
    $$ROOT_PWD/src/widgetdevolucao.h \
    $$ROOT_PWD/src/widgetestoqueproduto.h \
    $$ROOT_PWD/src/widgetestoques.h \
    $$ROOT_PWD/src/widgetfinanceirocompra.h \
    $$ROOT_PWD/src/widgetfinanceirocontas.h \
    $$ROOT_PWD/src/widgetfinanceirofluxocaixa.h \
    $$ROOT_PWD/src/widgetgalpao.h \
    $$ROOT_PWD/src/widgetgalpaopeso.h \
    $$ROOT_PWD/src/widgetgare.h \
    $$ROOT_PWD/src/widgetgraficos.h \
    $$ROOT_PWD/src/widgetlogisticaagendarcoleta.h \
    $$ROOT_PWD/src/widgetlogisticaagendarentrega.h \
    $$ROOT_PWD/src/widgetlogisticacalendario.h \
    $$ROOT_PWD/src/widgetlogisticacaminhao.h \
    $$ROOT_PWD/src/widgetlogisticacoleta.h \
    $$ROOT_PWD/src/widgetlogisticaentregas.h \
    $$ROOT_PWD/src/widgetlogisticaentregues.h \
    $$ROOT_PWD/src/widgetlogisticarecebimento.h \
    $$ROOT_PWD/src/widgetlogisticarepresentacao.h \
    $$ROOT_PWD/src/widgetnfedistribuicao.h \
    $$ROOT_PWD/src/widgetnfeentrada.h \
    $$ROOT_PWD/src/widgetnfesaida.h \
    $$ROOT_PWD/src/widgetorcamento.h \
    $$ROOT_PWD/src/widgetpagamentos.h \
    $$ROOT_PWD/src/widgetrelatorio.h \
    $$ROOT_PWD/src/widgetrh.h \
    $$ROOT_PWD/src/widgetvenda.h \
    $$ROOT_PWD/src/xml.h \
    $$ROOT_PWD/src/xml_viewer.h

FORMS += \
    $$ROOT_PWD/ui/anteciparrecebimento.ui \
    $$ROOT_PWD/ui/baixaorcamento.ui \
    $$ROOT_PWD/ui/cadastrarnfe.ui \
    $$ROOT_PWD/ui/cadastroStaccatoOff.ui \
    $$ROOT_PWD/ui/cadastrocliente.ui \
    $$ROOT_PWD/ui/cadastrofornecedor.ui \
    $$ROOT_PWD/ui/cadastrofuncionario.ui \
    $$ROOT_PWD/ui/cadastroloja.ui \
    $$ROOT_PWD/ui/cadastroncm.ui \
    $$ROOT_PWD/ui/cadastropagamento.ui \
    $$ROOT_PWD/ui/cadastroproduto.ui \
    $$ROOT_PWD/ui/cadastroprofissional.ui \
    $$ROOT_PWD/ui/cadastrotransportadora.ui \
    $$ROOT_PWD/ui/cadastrousuario.ui \
    $$ROOT_PWD/ui/calculofrete.ui \
    $$ROOT_PWD/ui/cancelaproduto.ui \
    $$ROOT_PWD/ui/collapsiblewidget.ui \
    $$ROOT_PWD/ui/compraavulsa.ui \
    $$ROOT_PWD/ui/comprovantes.ui \
    $$ROOT_PWD/ui/contas.ui \
    $$ROOT_PWD/ui/devolucao.ui \
    $$ROOT_PWD/ui/estoque.ui \
    $$ROOT_PWD/ui/followup.ui \
    $$ROOT_PWD/ui/importaprodutos.ui \
    $$ROOT_PWD/ui/importarxml.ui \
    $$ROOT_PWD/ui/inputdialog.ui \
    $$ROOT_PWD/ui/inputdialogconfirmacao.ui \
    $$ROOT_PWD/ui/inputdialogfinanceiro.ui \
    $$ROOT_PWD/ui/inputdialogproduto.ui \
    $$ROOT_PWD/ui/inserirlancamento.ui \
    $$ROOT_PWD/ui/inserirtransferencia.ui \
    $$ROOT_PWD/ui/logindialog.ui \
    $$ROOT_PWD/ui/mainwindow.ui \
    $$ROOT_PWD/ui/orcamento.ui \
    $$ROOT_PWD/ui/pagamentosdia.ui \
    $$ROOT_PWD/ui/precoestoque.ui \
    $$ROOT_PWD/ui/produtospendentes.ui \
    $$ROOT_PWD/ui/searchdialog.ui \
    $$ROOT_PWD/ui/sendmail.ui \
    $$ROOT_PWD/ui/tabcompras.ui \
    $$ROOT_PWD/ui/tabestoque.ui \
    $$ROOT_PWD/ui/tabfinanceiro.ui \
    $$ROOT_PWD/ui/tabgalpao.ui \
    $$ROOT_PWD/ui/tablogistica.ui \
    $$ROOT_PWD/ui/tabnfe.ui \
    $$ROOT_PWD/ui/userconfig.ui \
    $$ROOT_PWD/ui/validadedialog.ui \
    $$ROOT_PWD/ui/venda.ui \
    $$ROOT_PWD/ui/widgetcompraavulsa.ui \
    $$ROOT_PWD/ui/widgetcompraconfirmar.ui \
    $$ROOT_PWD/ui/widgetcompraconsumos.ui \
    $$ROOT_PWD/ui/widgetcompradevolucao.ui \
    $$ROOT_PWD/ui/widgetcomprafaturar.ui \
    $$ROOT_PWD/ui/widgetcompragerar.ui \
    $$ROOT_PWD/ui/widgetcomprahistorico.ui \
    $$ROOT_PWD/ui/widgetcomprapendentes.ui \
    $$ROOT_PWD/ui/widgetcompraresumo.ui \
    $$ROOT_PWD/ui/widgetconsistencia.ui \
    $$ROOT_PWD/ui/widgetdevolucao.ui \
    $$ROOT_PWD/ui/widgetestoqueproduto.ui \
    $$ROOT_PWD/ui/widgetestoques.ui \
    $$ROOT_PWD/ui/widgetfinanceirocompra.ui \
    $$ROOT_PWD/ui/widgetfinanceirocontas.ui \
    $$ROOT_PWD/ui/widgetfinanceirofluxocaixa.ui \
    $$ROOT_PWD/ui/widgetgalpao.ui \
    $$ROOT_PWD/ui/widgetgalpaopeso.ui \
    $$ROOT_PWD/ui/widgetgare.ui \
    $$ROOT_PWD/ui/widgetgraficos.ui \
    $$ROOT_PWD/ui/widgetlogisticaagendarcoleta.ui \
    $$ROOT_PWD/ui/widgetlogisticaagendarentrega.ui \
    $$ROOT_PWD/ui/widgetlogisticacalendario.ui \
    $$ROOT_PWD/ui/widgetlogisticacaminhao.ui \
    $$ROOT_PWD/ui/widgetlogisticacoleta.ui \
    $$ROOT_PWD/ui/widgetlogisticaentregas.ui \
    $$ROOT_PWD/ui/widgetlogisticaentregues.ui \
    $$ROOT_PWD/ui/widgetlogisticarecebimento.ui \
    $$ROOT_PWD/ui/widgetlogisticarepresentacao.ui \
    $$ROOT_PWD/ui/widgetnfedistribuicao.ui \
    $$ROOT_PWD/ui/widgetnfeentrada.ui \
    $$ROOT_PWD/ui/widgetnfesaida.ui \
    $$ROOT_PWD/ui/widgetorcamento.ui \
    $$ROOT_PWD/ui/widgetpagamentos.ui \
    $$ROOT_PWD/ui/widgetrelatorio.ui \
    $$ROOT_PWD/ui/widgetrh.ui \
    $$ROOT_PWD/ui/widgetvenda.ui \
    $$ROOT_PWD/ui/xml_viewer.ui
