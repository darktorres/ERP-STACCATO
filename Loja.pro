#-------------------------------------------------
#
# Project created by QtCreator 2014-12-04T14:47:28
#
#-------------------------------------------------

!versionAtLeast(QT_VERSION, 5.15.0) {
    error("Use Qt 5.15 ou mais novo")
}

TARGET = Loja
TEMPLATE = app

include(QtXlsxWriter/src/xlsx/qtxlsx.pri)
include(QSimpleUpdater/qsimpleupdater.pri)
include(LimeReport-1.5.68/limereport/limereport.pri)

QT *= core gui sql network xml charts widgets

DEFINES *= QT_DEPRECATED_WARNINGS
# VERSION is empty
DEFINES *= APP_VERSION=\"\\\"$${VERSION}\\\"\"

CONFIG *= c++17 warn_on

PRECOMPILED_HEADER = pch.h
CONFIG *= precompile_header

message($$QMAKESPEC)

win32 {
    QMAKE_TARGET_COMPANY = Staccato Revestimentos
    QMAKE_TARGET_PRODUCT = ERP
    QMAKE_TARGET_DESCRIPTION = ERP da Staccato Revestimentos
    QMAKE_TARGET_COPYRIGHT = Rodrigo Torres

    RC_ICONS = Staccato.ico
}

win32-msvc { LIBS += -L$$_PRO_FILE_PWD_/OpenSSL-1.1-Win32 -llibcrypto }

win32-g++ { LIBS += -L$$_PRO_FILE_PWD_/OpenSSL-1.1-Win32 -llibcrypto-1_1 }

contains(CONFIG, deploy) {
    message(deploy)
    DEFINES *= DEPLOY

    win32-msvc {
        QMAKE_CXXFLAGS_RELEASE *= /O2
    }

    *-g++ {
        QMAKE_CXXFLAGS_RELEASE *= -O3
        QMAKE_LFLAGS_RELEASE *= -O3
    }
} else {
    win32-msvc {
        QMAKE_CXXFLAGS_DEBUG -= -O2
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS_DEBUG *= /Od
        QMAKE_CXXFLAGS_RELEASE *= /Od
    }

    *-g++ {
        QMAKE_CXXFLAGS_DEBUG *= -O0
        QMAKE_CXXFLAGS_RELEASE *= -O0
        QMAKE_LFLAGS_DEBUG *= -O0
        QMAKE_LFLAGS_RELEASE *= -O0
    }
}

win32-msvc {
   QMAKE_CXXFLAGS += /permissive-
}

linux-g++ {
    QMAKE_LFLAGS *= -fuse-ld=gold
}

linux-clang {
    QMAKE_LFLAGS *= -fuse-ld=lld-13
}

win32-g++ { # ccache is not compatible with MSVC
    exists($(QTDIR)/bin/ccache.exe) {
        message("using ccache")
        QMAKE_CC = ccache $$QMAKE_CC
        QMAKE_CXX = ccache $$QMAKE_CXX
        message($$QMAKE_CC)
        message($$QMAKE_CXX)

        QMAKE_CXXFLAGS += -fpch-preprocess # must also set sloppiness to pch_defines,time_macros in ccache.conf
    }
}

linux {
    CCACHE_BIN = $$system(which ccache)

    !isEmpty(CCACHE_BIN) {
        message("using ccache")
        QMAKE_CC = ccache $$QMAKE_CC
        QMAKE_CXX = ccache $$QMAKE_CXX
        message($$QMAKE_CC)
        message($$QMAKE_CXX)
    }
}

INCLUDEPATH += $$PWD/src

RESOURCES += \
    qrs/resources.qrc

SOURCES += \
    src/acbr.cpp \
    src/acbrlib.cpp \
    src/anteciparrecebimento.cpp \
    src/application.cpp \
    src/baixaorcamento.cpp \
    src/cadastrarnfe.cpp \
    src/cadastrocliente.cpp \
    src/cadastrofornecedor.cpp \
    src/cadastroloja.cpp \
    src/cadastroncm.cpp \
    src/cadastropagamento.cpp \
    src/cadastroproduto.cpp \
    src/cadastroprofissional.cpp \
    src/cadastrostaccatooff.cpp \
    src/cadastrotransportadora.cpp \
    src/cadastrousuario.cpp \
    src/calculofrete.cpp \
    src/cancelaproduto.cpp \
    src/cepcompleter.cpp \
    src/charttooltip.cpp \
    src/chartview.cpp \
    src/checkboxdelegate.cpp \
    src/cnab.cpp \
    src/collapsiblewidget.cpp \
    src/combobox.cpp \
    src/comboboxdelegate.cpp \
    src/comprovantes.cpp \
    src/contas.cpp \
    src/dateformatdelegate.cpp \
    src/devolucao.cpp \
    src/doubledelegate.cpp \
    src/doublespinbox.cpp \
    src/editdelegate.cpp \
    src/estoque.cpp \
    src/estoqueitem.cpp \
    src/estoqueprazoproxymodel.cpp \
    src/estoqueproxymodel.cpp \
    src/excel.cpp \
    src/file.cpp \
    src/financeiroproxymodel.cpp \
    src/followup.cpp \
    src/followupproxymodel.cpp \
    src/graphicsview.cpp \
    src/importaprodutos.cpp \
    src/importaprodutosproxymodel.cpp \
    src/importarxml.cpp \
    src/importatabelaibpt.cpp \
    src/inputdialog.cpp \
    src/inputdialogconfirmacao.cpp \
    src/inputdialogfinanceiro.cpp \
    src/inputdialogproduto.cpp \
    src/inserirlancamento.cpp \
    src/inserirtransferencia.cpp \
    src/itembox.cpp \
    src/itemboxdelegate.cpp \
    src/lineedit.cpp \
    src/lineeditcep.cpp \
    src/lineeditdelegate.cpp \
    src/lineedittel.cpp \
    src/log.cpp \
    src/logindialog.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/nfedistribuicao.cpp \
    src/nfeproxymodel.cpp \
    src/noeditdelegate.cpp \
    src/orcamento.cpp \
    src/orcamentoproxymodel.cpp \
    src/pagamentosdia.cpp \
    src/palletitem.cpp \
    src/pdf.cpp \
    src/porcentagemdelegate.cpp \
    src/precoestoque.cpp \
    src/produtoproxymodel.cpp \
    src/produtospendentes.cpp \
    src/reaisdelegate.cpp \
    src/registeraddressdialog.cpp \
    src/registerdialog.cpp \
    src/scrollarea.cpp \
    src/searchdialog.cpp \
    src/sendmail.cpp \
    src/smtp.cpp \
    src/sortfilterproxymodel.cpp \
    src/sql.cpp \
    src/sqlquery.cpp \
    src/sqlquerymodel.cpp \
    src/sqltablemodel.cpp \
    src/sqltreemodel.cpp \
    src/tabcompras.cpp \
    src/tabestoque.cpp \
    src/tabfinanceiro.cpp \
    src/tabgalpao.cpp \
    src/tableview.cpp \
    src/tablogistica.cpp \
    src/tabnfe.cpp \
    src/treeview.cpp \
    src/user.cpp \
    src/userconfig.cpp \
    src/validadedialog.cpp \
    src/venda.cpp \
    src/vendaproxymodel.cpp \
    src/widgetcompraconfirmar.cpp \
    src/widgetcompraconsumos.cpp \
    src/widgetcompradevolucao.cpp \
    src/widgetcomprafaturar.cpp \
    src/widgetcompragerar.cpp \
    src/widgetcomprapendentes.cpp \
    src/widgetcompraresumo.cpp \
    src/widgetconsistencia.cpp \
    src/widgetdevolucao.cpp \
    src/widgetestoqueproduto.cpp \
    src/widgetestoques.cpp \
    src/widgetfinanceirocompra.cpp \
    src/widgetfinanceirocontas.cpp \
    src/widgetfinanceirofluxocaixa.cpp \
    src/widgetgalpao.cpp \
    src/widgetgalpaopeso.cpp \
    src/widgetgare.cpp \
    src/widgetgraficos.cpp \
    src/widgethistoricocompra.cpp \
    src/widgetlogisticaagendarcoleta.cpp \
    src/widgetlogisticaagendarentrega.cpp \
    src/widgetlogisticacalendario.cpp \
    src/widgetlogisticacaminhao.cpp \
    src/widgetlogisticacoleta.cpp \
    src/widgetlogisticaentregas.cpp \
    src/widgetlogisticaentregues.cpp \
    src/widgetlogisticarecebimento.cpp \
    src/widgetlogisticarepresentacao.cpp \
    src/widgetnfeentrada.cpp \
    src/widgetnfesaida.cpp \
    src/widgetorcamento.cpp \
    src/widgetpagamentos.cpp \
    src/widgetrelatorio.cpp \
    src/widgetrh.cpp \
    src/widgetvenda.cpp \
    src/xml.cpp \
    src/xml_viewer.cpp

HEADERS  += \
    src/acbr.h \
    src/acbrlib.h \
    src/anteciparrecebimento.h \
    src/application.h \
    src/baixaorcamento.h \
    src/cadastrarnfe.h \
    src/cadastrocliente.h \
    src/cadastrofornecedor.h \
    src/cadastroloja.h \
    src/cadastroncm.h \
    src/cadastropagamento.h \
    src/cadastroproduto.h \
    src/cadastroprofissional.h \
    src/cadastrostaccatooff.h \
    src/cadastrotransportadora.h \
    src/cadastrousuario.h \
    src/calculofrete.h \
    src/cancelaproduto.h \
    src/cepcompleter.h \
    src/charttooltip.h \
    src/chartview.h \
    src/checkboxdelegate.h \
    src/cnab.h \
    src/collapsiblewidget.h \
    src/combobox.h \
    src/comboboxdelegate.h \
    src/comprovantes.h \
    src/contas.h \
    src/dateformatdelegate.h \
    src/devolucao.h \
    src/doubledelegate.h \
    src/doublespinbox.h \
    src/editdelegate.h \
    src/estoque.h \
    src/estoqueitem.h \
    src/estoqueprazoproxymodel.h \
    src/estoqueproxymodel.h \
    src/excel.h \
    src/file.h \
    src/financeiroproxymodel.h \
    src/followup.h \
    src/followupproxymodel.h \
    src/graphicsview.h \
    src/importaprodutos.h \
    src/importaprodutosproxymodel.h \
    src/importarxml.h \
    src/importatabelaibpt.h \
    src/inputdialog.h \
    src/inputdialogconfirmacao.h \
    src/inputdialogfinanceiro.h \
    src/inputdialogproduto.h \
    src/inserirlancamento.h \
    src/inserirtransferencia.h \
    src/itembox.h \
    src/itemboxdelegate.h \
    src/lineedit.h \
    src/lineeditcep.h \
    src/lineeditdelegate.h \
    src/lineedittel.h \
    src/log.h \
    src/logindialog.h \
    src/mainwindow.h \
    src/nfedistribuicao.h \
    src/nfeproxymodel.h \
    src/noeditdelegate.h \
    src/orcamento.h \
    src/orcamentoproxymodel.h \
    src/pagamentosdia.h \
    src/palletitem.h \
    src/pdf.h \
    src/porcentagemdelegate.h \
    src/precoestoque.h \
    src/produtoproxymodel.h \
    src/produtospendentes.h \
    src/reaisdelegate.h \
    src/registeraddressdialog.h \
    src/registerdialog.h \
    src/scrollarea.h \
    src/searchdialog.h \
    src/sendmail.h \
    src/smtp.h \
    src/sortfilterproxymodel.h \
    src/sql.h \
    src/sqlquery.h \
    src/sqlquerymodel.h \
    src/sqltablemodel.h \
    src/sqltreemodel.h \
    src/tabcompras.h \
    src/tabestoque.h \
    src/tabfinanceiro.h \
    src/tabgalpao.h \
    src/tableview.h \
    src/tablogistica.h \
    src/tabnfe.h \
    src/treeview.h \
    src/user.h \
    src/userconfig.h \
    src/validadedialog.h \
    src/venda.h \
    src/vendaproxymodel.h \
    src/widgetcompraconfirmar.h \
    src/widgetcompraconsumos.h \
    src/widgetcompradevolucao.h \
    src/widgetcomprafaturar.h \
    src/widgetcompragerar.h \
    src/widgetcomprapendentes.h \
    src/widgetcompraresumo.h \
    src/widgetconsistencia.h \
    src/widgetdevolucao.h \
    src/widgetestoqueproduto.h \
    src/widgetestoques.h \
    src/widgetfinanceirocompra.h \
    src/widgetfinanceirocontas.h \
    src/widgetfinanceirofluxocaixa.h \
    src/widgetgalpao.h \
    src/widgetgalpaopeso.h \
    src/widgetgare.h \
    src/widgetgraficos.h \
    src/widgethistoricocompra.h \
    src/widgetlogisticaagendarcoleta.h \
    src/widgetlogisticaagendarentrega.h \
    src/widgetlogisticacalendario.h \
    src/widgetlogisticacaminhao.h \
    src/widgetlogisticacoleta.h \
    src/widgetlogisticaentregas.h \
    src/widgetlogisticaentregues.h \
    src/widgetlogisticarecebimento.h \
    src/widgetlogisticarepresentacao.h \
    src/widgetnfeentrada.h \
    src/widgetnfesaida.h \
    src/widgetorcamento.h \
    src/widgetpagamentos.h \
    src/widgetrelatorio.h \
    src/widgetrh.h \
    src/widgetvenda.h \
    src/xml.h \
    src/xml_viewer.h

FORMS += \
    ui/anteciparrecebimento.ui \
    ui/baixaorcamento.ui \
    ui/cadastrarnfe.ui \
    ui/cadastroStaccatoOff.ui \
    ui/cadastrocliente.ui \
    ui/cadastrofornecedor.ui \
    ui/cadastroloja.ui \
    ui/cadastroncm.ui \
    ui/cadastropagamento.ui \
    ui/cadastroproduto.ui \
    ui/cadastroprofissional.ui \
    ui/cadastrotransportadora.ui \
    ui/cadastrousuario.ui \
    ui/calculofrete.ui \
    ui/cancelaproduto.ui \
    ui/collapsiblewidget.ui \
    ui/comprovantes.ui \
    ui/contas.ui \
    ui/devolucao.ui \
    ui/estoque.ui \
    ui/followup.ui \
    ui/importaprodutos.ui \
    ui/importarxml.ui \
    ui/inputdialog.ui \
    ui/inputdialogconfirmacao.ui \
    ui/inputdialogfinanceiro.ui \
    ui/inputdialogproduto.ui \
    ui/inserirlancamento.ui \
    ui/inserirtransferencia.ui \
    ui/logindialog.ui \
    ui/mainwindow.ui \
    ui/nfedistribuicao.ui \
    ui/orcamento.ui \
    ui/pagamentosdia.ui \
    ui/precoestoque.ui \
    ui/produtospendentes.ui \
    ui/searchdialog.ui \
    ui/sendmail.ui \
    ui/tabcompras.ui \
    ui/tabestoque.ui \
    ui/tabfinanceiro.ui \
    ui/tabgalpao.ui \
    ui/tablogistica.ui \
    ui/tabnfe.ui \
    ui/userconfig.ui \
    ui/validadedialog.ui \
    ui/venda.ui \
    ui/widgetcompraconfirmar.ui \
    ui/widgetcompraconsumos.ui \
    ui/widgetcompradevolucao.ui \
    ui/widgetcomprafaturar.ui \
    ui/widgetcompragerar.ui \
    ui/widgetcomprapendentes.ui \
    ui/widgetcompraresumo.ui \
    ui/widgetconsistencia.ui \
    ui/widgetdevolucao.ui \
    ui/widgetestoqueproduto.ui \
    ui/widgetestoques.ui \
    ui/widgetfinanceirocompra.ui \
    ui/widgetfinanceirocontas.ui \
    ui/widgetfinanceirofluxocaixa.ui \
    ui/widgetgalpao.ui \
    ui/widgetgalpaopeso.ui \
    ui/widgetgare.ui \
    ui/widgetgraficos.ui \
    ui/widgethistoricocompra.ui \
    ui/widgetlogisticaagendarcoleta.ui \
    ui/widgetlogisticaagendarentrega.ui \
    ui/widgetlogisticacalendario.ui \
    ui/widgetlogisticacaminhao.ui \
    ui/widgetlogisticacoleta.ui \
    ui/widgetlogisticaentregas.ui \
    ui/widgetlogisticaentregues.ui \
    ui/widgetlogisticarecebimento.ui \
    ui/widgetlogisticarepresentacao.ui \
    ui/widgetnfeentrada.ui \
    ui/widgetnfesaida.ui \
    ui/widgetorcamento.ui \
    ui/widgetpagamentos.ui \
    ui/widgetrelatorio.ui \
    ui/widgetrh.ui \
    ui/widgetvenda.ui \
    ui/xml_viewer.ui
