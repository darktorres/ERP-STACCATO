#-------------------------------------------------
#
# Project created by QtCreator 2014-12-04T14:47:28
#
#-------------------------------------------------

!versionAtLeast(QT_VERSION, 5.14.0) {
    error("Use Qt 5.14 ou mais novo")
}

win32-msvc* {
    error("Não compatível com VisualStudio, use MinGW")
}

TARGET = Loja
TEMPLATE = app
VERSION = 0.8.88

include(QtXlsxWriter/src/xlsx/qtxlsx.pri)
include(QSimpleUpdater/qsimpleupdater.pri)
include(LimeReport-1.5.56/limereport/limereport.pri)

QT *= core gui sql network xml charts widgets

DEFINES *= QT_DEPRECATED_WARNINGS
DEFINES *= APP_VERSION=\"\\\"$${VERSION}\\\"\"

versionAtLeast(QT_VERSION, 5.12){
    CONFIG *= c++17
    } else {
    CONFIG *= c++1z
    }

message($$QMAKESPEC)

win32{
    QMAKE_TARGET_COMPANY = Staccato Revestimentos
    QMAKE_TARGET_PRODUCT = ERP
    QMAKE_TARGET_DESCRIPTION = ERP da Staccato Revestimentos
    QMAKE_TARGET_COPYRIGHT = Rodrigo Torres

    RC_ICONS = Staccato.ico

    LIBS += -L$$_PRO_FILE_PWD_/OpenSSL-1.1-Win32 -llibcrypto-1_1
}

contains(CONFIG, deploy){
    message(deploy)
    DEFINES *= DEPLOY
    QMAKE_CXXFLAGS_RELEASE *= -Ofast -flto
    QMAKE_LFLAGS_RELEASE *= -O3 -fuse-linker-plugin
} else{
    QMAKE_CXXFLAGS_DEBUG *= -O0
    QMAKE_CXXFLAGS_RELEASE *= -O0
    QMAKE_LFLAGS_DEBUG *= -O0
    QMAKE_LFLAGS_RELEASE *= -O0
}

    PRECOMPILED_HEADER = pch.h
    CONFIG *= precompile_header

*-g++{
    QMAKE_CXXFLAGS *= -Wall -Wextra -Wpedantic -Wfloat-equal -Wnarrowing
    QMAKE_CXXFLAGS *= -Wnull-dereference -Wold-style-cast -Wdouble-promotion -Wformat=2 -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wshadow=local
}

*-clang{
#    QMAKE_CXXFLAGS *= -Weverything -Wno-reserved-id-macro -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-undef -Wno-padded -Wno-sign-conversion -Wno-deprecated -Wno-covered-switch-default
#    QMAKE_CXXFLAGS *= -Wno-undefined-reinterpret-cast -Wno-weak-vtables -Wno-exit-time-destructors -Wno-used-but-marked-unused -Wno-inconsistent-missing-destructor-override -Wno-redundant-parens
#    QMAKE_CXXFLAGS *= -Wno-shift-sign-overflow -Wno-non-virtual-dtor -Wno-conversion -Wno-global-constructors -Wno-switch-enum -Wno-missing-prototypes -Wno-shadow-field-in-constructor
#    QMAKE_CXXFLAGS *= -Wno-shadow -Wno-shadow-field
}

linux-g++{
    QMAKE_CC = gcc-10
    QMAKE_CXX = g++-10

    QMAKE_LFLAGS *= -fuse-ld=gold

    QMAKE_CXXFLAGS *= -Wno-deprecated-copy

    #QMAKE_CXXFLAGS *= -flto
    #QMAKE_LFLAGS *= -flto -fuse-linker-plugin
}

linux-clang{
    QMAKE_CC = clang-9
    QMAKE_CXX = clang++-9

    QMAKE_LFLAGS *= -fuse-ld=lld-9

    #QMAKE_CXXFLAGS *= -flto=thin
    #QMAKE_LFLAGS *= -flto=thin
}

win32{
    exists($(QTDIR)/bin/ccache.exe){
        message("using ccache")
        QMAKE_CC = ccache $$QMAKE_CC
        QMAKE_CXX = ccache $$QMAKE_CXX
        message($$QMAKE_CC)
        message($$QMAKE_CXX)

        QMAKE_CXXFLAGS += -fpch-preprocess # must also set sloppiness to pch_defines,time_macros in ccache.conf
    }
}

linux{
    CCACHE_BIN = $$system(which ccache)

    !isEmpty(CCACHE_BIN){
        message("using ccache")
        QMAKE_CC = ccache $$QMAKE_CC
        QMAKE_CXX = ccache $$QMAKE_CXX
        message($$QMAKE_CC)
        message($$QMAKE_CXX)
    }
}

RESOURCES += \
    qrs/resources.qrc

SOURCES += \
    src/acbr.cpp \
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
    src/editdelegate.cpp \
    src/estoque.cpp \
    src/estoqueprazoproxymodel.cpp \
    src/estoqueproxymodel.cpp \
    src/excel.cpp \
    src/financeiroproxymodel.cpp \
    src/followup.cpp \
    src/followupproxymodel.cpp \
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
    src/lineeditcep.cpp \
    src/lineeditdecimal.cpp \
    src/lineeditdelegate.cpp \
    src/lineedittel.cpp \
    src/log.cpp \
    src/logindialog.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/noeditdelegate.cpp \
    src/orcamento.cpp \
    src/orcamentoproxymodel.cpp \
    src/pagamentosdia.cpp \
    src/pdf.cpp \
    src/porcentagemdelegate.cpp \
    src/precoestoque.cpp \
    src/produtospendentes.cpp \
    src/reaisdelegate.cpp \
    src/registeraddressdialog.cpp \
    src/registerdialog.cpp \
    src/searchdialog.cpp \
    src/searchdialogproxymodel.cpp \
    src/sendmail.cpp \
    src/smtp.cpp \
    src/sortfilterproxymodel.cpp \
    src/sql.cpp \
    src/sqlquerymodel.cpp \
    src/sqltablemodel.cpp \
    src/sqltreemodel.cpp \
    src/tableview.cpp \
    src/treeview.cpp \
    src/userconfig.cpp \
    src/usersession.cpp \
    src/validadedialog.cpp \
    src/venda.cpp \
    src/vendaproxymodel.cpp \
    src/widgetcompra.cpp \
    src/widgetcompraconfirmar.cpp \
    src/widgetcompraconsumos.cpp \
    src/widgetcompradevolucao.cpp \
    src/widgetcomprafaturar.cpp \
    src/widgetcompragerar.cpp \
    src/widgetcomprapendentes.cpp \
    src/widgetcompraresumo.cpp \
    src/widgetconsistencia.cpp \
    src/widgetestoque.cpp \
    src/widgetfinanceiro.cpp \
    src/widgetfinanceirocompra.cpp \
    src/widgetfinanceirocontas.cpp \
    src/widgetfinanceirofluxocaixa.cpp \
    src/widgetgare.cpp \
    src/widgetgraficos.cpp \
    src/widgethistoricocompra.cpp \
    src/widgetlogistica.cpp \
    src/widgetlogisticaagendarcoleta.cpp \
    src/widgetlogisticaagendarentrega.cpp \
    src/widgetlogisticacalendario.cpp \
    src/widgetlogisticacaminhao.cpp \
    src/widgetlogisticacoleta.cpp \
    src/widgetlogisticaentregas.cpp \
    src/widgetlogisticaentregues.cpp \
    src/widgetlogisticarecebimento.cpp \
    src/widgetlogisticarepresentacao.cpp \
    src/widgetnfe.cpp \
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
    src/editdelegate.h \
    src/estoque.h \
    src/estoqueprazoproxymodel.h \
    src/estoqueproxymodel.h \
    src/excel.h \
    src/financeiroproxymodel.h \
    src/followup.h \
    src/followupproxymodel.h \
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
    src/lineeditcep.h \
    src/lineeditdecimal.h \
    src/lineeditdelegate.h \
    src/lineedittel.h \
    src/log.h \
    src/logindialog.h \
    src/mainwindow.h \
    src/noeditdelegate.h \
    src/orcamento.h \
    src/orcamentoproxymodel.h \
    src/pagamentosdia.h \
    src/pdf.h \
    src/porcentagemdelegate.h \
    src/precoestoque.h \
    src/produtospendentes.h \
    src/reaisdelegate.h \
    src/registeraddressdialog.h \
    src/registerdialog.h \
    src/searchdialog.h \
    src/searchdialogproxymodel.h \
    src/sendmail.h \
    src/smtp.h \
    src/sortfilterproxymodel.h \
    src/sql.h \
    src/sqlquerymodel.h \
    src/sqltablemodel.h \
    src/sqltreemodel.h \
    src/tableview.h \
    src/treeview.h \
    src/userconfig.h \
    src/usersession.h \
    src/validadedialog.h \
    src/venda.h \
    src/vendaproxymodel.h \
    src/widgetcompra.h \
    src/widgetcompraconfirmar.h \
    src/widgetcompraconsumos.h \
    src/widgetcompradevolucao.h \
    src/widgetcomprafaturar.h \
    src/widgetcompragerar.h \
    src/widgetcomprapendentes.h \
    src/widgetcompraresumo.h \
    src/widgetconsistencia.h \
    src/widgetestoque.h \
    src/widgetfinanceiro.h \
    src/widgetfinanceirocompra.h \
    src/widgetfinanceirocontas.h \
    src/widgetfinanceirofluxocaixa.h \
    src/widgetgare.h \
    src/widgetgraficos.h \
    src/widgethistoricocompra.h \
    src/widgetlogistica.h \
    src/widgetlogisticaagendarcoleta.h \
    src/widgetlogisticaagendarentrega.h \
    src/widgetlogisticacalendario.h \
    src/widgetlogisticacaminhao.h \
    src/widgetlogisticacoleta.h \
    src/widgetlogisticaentregas.h \
    src/widgetlogisticaentregues.h \
    src/widgetlogisticarecebimento.h \
    src/widgetlogisticarepresentacao.h \
    src/widgetnfe.h \
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
    ui/cnab.ui \
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
    ui/orcamento.ui \
    ui/pagamentosdia.ui \
    ui/precoestoque.ui \
    ui/produtospendentes.ui \
    ui/searchdialog.ui \
    ui/sendmail.ui \
    ui/userconfig.ui \
    ui/validadedialog.ui \
    ui/venda.ui \
    ui/widgetcompra.ui \
    ui/widgetcompraconfirmar.ui \
    ui/widgetcompraconsumos.ui \
    ui/widgetcompradevolucao.ui \
    ui/widgetcomprafaturar.ui \
    ui/widgetcompragerar.ui \
    ui/widgetcomprapendentes.ui \
    ui/widgetcompraresumo.ui \
    ui/widgetconsistencia.ui \
    ui/widgetestoque.ui \
    ui/widgetfinanceiro.ui \
    ui/widgetfinanceirocompra.ui \
    ui/widgetfinanceirocontas.ui \
    ui/widgetfinanceirofluxocaixa.ui \
    ui/widgetgare.ui \
    ui/widgetgraficos.ui \
    ui/widgethistoricocompra.ui \
    ui/widgetlogistica.ui \
    ui/widgetlogisticaagendarcoleta.ui \
    ui/widgetlogisticaagendarentrega.ui \
    ui/widgetlogisticacalendario.ui \
    ui/widgetlogisticacaminhao.ui \
    ui/widgetlogisticacoleta.ui \
    ui/widgetlogisticaentregas.ui \
    ui/widgetlogisticaentregues.ui \
    ui/widgetlogisticarecebimento.ui \
    ui/widgetlogisticarepresentacao.ui \
    ui/widgetnfe.ui \
    ui/widgetnfeentrada.ui \
    ui/widgetnfesaida.ui \
    ui/widgetorcamento.ui \
    ui/widgetpagamentos.ui \
    ui/widgetrelatorio.ui \
    ui/widgetrh.ui \
    ui/widgetvenda.ui \
    ui/xml_viewer.ui
