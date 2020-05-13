#include "cadastroncm.h"
#include "ui_cadastroncm.h"

#include "application.h"
#include "porcentagemdelegate.h"

#include <QDebug>

CadastroNCM::CadastroNCM(QWidget *parent) : QDialog(parent), ui(new Ui::CadastroNCM) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &CadastroNCM::on_lineEditBusca_textChanged);
  connect(ui->pushButtonAdicionar, &QPushButton::clicked, this, &CadastroNCM::on_pushButtonAdicionar_clicked);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &CadastroNCM::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroNCM::on_pushButtonRemover_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CadastroNCM::on_pushButtonSalvar_clicked);

  setWindowFlags(Qt::Window);

  setupTables();
}

CadastroNCM::~CadastroNCM() { delete ui; }

void CadastroNCM::setupTables() {
  model.setTable("ncm");

  model.setSort("ncm");

  model.setHeaderData("ncm", "NCM");
  model.setHeaderData("mva4", "MVA 4%");
  model.setHeaderData("mva12", "MVA 12%");
  model.setHeaderData("aliq", "Alíq. ICMS");

  model.setFilter("");

  if (not model.select()) { return; }

  ui->table->setModel(&model);

  ui->table->hideColumn("idncm");

  ui->table->setItemDelegateForColumn("mva4", new PorcentagemDelegate(false, this));
  ui->table->setItemDelegateForColumn("mva12", new PorcentagemDelegate(false, this));
  ui->table->setItemDelegateForColumn("aliq", new PorcentagemDelegate(false, this));
}

void CadastroNCM::on_pushButtonSalvar_clicked() {
  for (int row = 0; row < model.rowCount(); ++row) {
    if (model.data(row, "ncm").toString().length() != 8) { return qApp->enqueueError("NCM deve ter 8 dígitos!", this); }
  }

  if (not model.submitAll()) { return; }

  qApp->enqueueInformation("Dados atualizados!", this);
  close();
}

void CadastroNCM::on_pushButtonCancelar_clicked() { close(); }

void CadastroNCM::on_lineEditBusca_textChanged(const QString &text) { model.setFilter("ncm LIKE '%" + text + "%'"); }

void CadastroNCM::on_pushButtonAdicionar_clicked() {
  const int row = model.insertRowAtEnd();
  ui->table->selectRow(row);
}

void CadastroNCM::on_pushButtonRemover_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  for (const auto &index : selection) { model.removeRow(index.row()); }

  if (not model.submitAll()) { return; }
}

// TODO: avisar após digitar NCM se ele já estiver cadastrado (o banco de dados não vai permitir cadastrar duplicado mas a mensagem de
// erro dele não é amigável
