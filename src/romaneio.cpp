#include "romaneio.h"

#if __has_include("lrreportengine.h")
#include "lrreportengine.h"
#endif

#include "application.h"
#include "file.h"
#include "sql.h"
#include "sqlquery.h"
#include "sqlquerymodel.h"
#include "user.h"

#include <QDesktopServices>
#include <QDir>
#include <QSqlError>
#include <QStringList>
#include <QWidget>

QString Romaneio::gerar(const QString &idVenda, const QDate &dataEntrega, QWidget *parent) {
  if (idVenda.isEmpty()) { throw RuntimeError("Nenhuma venda selecionada!", parent); }

  // -------------------------------------------------------------------------

  const QString folderKey = User::getSetting("User/EntregasPdfFolder").toString();

  if (folderKey.isEmpty()) { throw RuntimeError("Não há uma pasta definida para salvar PDF. Por favor escolha uma nas configurações do ERP!", parent); }

  // -------------------------------------------------------------------------

  SqlQuery queryCliente;
  queryCliente.prepare("SELECT v.idProfissional, c.nome_razao, c.tel AS clienteTel, c.telCel AS clienteCel, p.tel AS profissionalTel, p.telCel AS profissionalCel FROM venda v LEFT JOIN cliente c ON "
                        "v.idCliente = c.idCliente LEFT JOIN profissional p ON v.idProfissional = p.idProfissional WHERE v.idVenda = :idVenda");
  queryCliente.bindValue(":idVenda", idVenda);

  if (not queryCliente.exec()) { throw RuntimeException("Erro buscando dados cliente: " + queryCliente.lastError().text(), parent); }

  if (not queryCliente.first()) { throw RuntimeException("Não foram encontrados dados para a Venda: '" + idVenda + "'"); }

  const QString cliente = queryCliente.value("nome_razao").toString();

  QString telefones;

  if (queryCliente.value("idProfissional").toInt() != 1) {
    const QString profissionalTel = queryCliente.value("profissionalTel").toString();
    const QString profissionalCel = queryCliente.value("profissionalCel").toString();

    if (not profissionalTel.isEmpty() and not profissionalCel.isEmpty()) {
      telefones = profissionalTel + " - " + profissionalCel;
    } else if (not profissionalTel.isEmpty()) {
      telefones = profissionalTel;
    } else if (not profissionalCel.isEmpty()) {
      telefones = profissionalCel;
    }
  }

  if (telefones.isEmpty()) {
    const QString clienteTel = queryCliente.value("clienteTel").toString();
    const QString clienteCel = queryCliente.value("clienteCel").toString();

    if (not clienteTel.isEmpty() and not clienteCel.isEmpty()) {
      telefones = clienteTel + " - " + clienteCel;
    } else if (not clienteTel.isEmpty()) {
      telefones = clienteTel;
    } else if (not clienteCel.isEmpty()) {
      telefones = clienteCel;
    }
  }

  // -------------------------------------------------------------------------

  SqlQuery queryEndereco;
  queryEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep FROM cliente_has_endereco WHERE idEndereco = (SELECT idEnderecoEntrega FROM venda WHERE idVenda = :idVenda)");
  queryEndereco.bindValue(":idVenda", idVenda);

  if (not queryEndereco.exec()) { throw RuntimeException("Erro buscando endereço: " + queryEndereco.lastError().text(), parent); }

  QString endereco;
  QString cep;

  if (queryEndereco.first()) {
    endereco = queryEndereco.value("logradouro").toString() + " " + queryEndereco.value("numero").toString() + " " + queryEndereco.value("complemento").toString() + " - " +
               queryEndereco.value("bairro").toString() + ", " + queryEndereco.value("cidade").toString();
    cep = queryEndereco.value("cep").toString();
  }

  // -------------------------------------------------------------------------

  SqlQueryModel modelRomaneio;
  modelRomaneio.setQuery(Sql::view_agendar_entrega(idVenda, "vp2.status NOT IN ('CANCELADO', 'DEVOLVIDO', 'ENTREGUE', 'QUEBRADO')") + " ORDER BY status, local, bloco, fornecedor, codComercial");
  modelRomaneio.select();

  if (modelRomaneio.rowCount() == 0) { throw RuntimeError("Não há itens para esta venda!", parent); }

  // -------------------------------------------------------------------------

#if __has_include("lrreportengine.h")
  const QString modelo = QDir::currentPath() + "/modelos/romaneio_separado.lrxml";

  File modeloFile(modelo);

  if (not modeloFile.exists()) { throw RuntimeException("Não encontrou o modelo do romaneio!", parent); }

  const QString fileName = folderKey + "/" + idVenda + "_romaneio.pdf";

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString(), parent); }

  file.close();

  LimeReport::ReportEngine report;
  auto *dm = report.dataManager();

  dm->addModel("produtos", &modelRomaneio, false);

  if (not report.loadFromFile(modelo)) { throw RuntimeException("Erro carregando modelo do romaneio!", parent); }

  QStringList nfeList;
  for (int i = 0; i < modelRomaneio.rowCount(); ++i) {
    const QString nfe = modelRomaneio.data(i, "nfeSaida").toString();
    if (not nfe.isEmpty() and not nfeList.contains(nfe)) { nfeList << nfe; }
  }
  dm->setReportVariable("nfe", nfeList.join(", "));

  dm->setReportVariable("idVenda", idVenda);
  dm->setReportVariable("cliente", cliente);
  dm->setReportVariable("telefones", telefones);
  dm->setReportVariable("endereco", endereco);
  dm->setReportVariable("cep", cep);
  dm->setReportVariable("dataEntrega", dataEntrega.toString("dd/MM/yy"));
  dm->setReportVariable("data", QDate::currentDate().toString("dd/MM/yyyy"));

  if (not report.printToPDF(fileName)) { throw RuntimeException("Erro gerando PDF do romaneio: " + report.lastError(), parent); }

  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(fileName))) { throw RuntimeException("Erro abrindo arquivo: " + fileName, parent); }

  return fileName;
#else
  throw RuntimeException("LimeReport desativado — não é possível gerar o romaneio!", parent);
#endif
}
