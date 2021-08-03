#include "validadedialog.h"
#include "ui_validadedialog.h"

#include "application.h"

ValidadeDialog::ValidadeDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ValidadeDialog) {
  ui->setupUi(this);

  ui->dateEdit->setDate(qApp->serverDate());

  setConnections();
}

ValidadeDialog::~ValidadeDialog() { delete ui; }

void ValidadeDialog::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &ValidadeDialog::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->spinBoxDias, qOverload<int>(&QSpinBox::valueChanged), this, &ValidadeDialog::on_spinBox_valueChanged, connectionType);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &ValidadeDialog::on_dateEdit_dateChanged, connectionType);
  connect(ui->checkBoxSemValidade, &QCheckBox::toggled, this, &ValidadeDialog::on_checkBoxSemValidade_toggled, connectionType);
}

void ValidadeDialog::on_pushButtonSalvar_clicked() {
  QDialog::accept();
  close();
}

void ValidadeDialog::on_spinBox_valueChanged(const int dias) { ui->dateEdit->setDate(qApp->serverDate().addDays(dias)); }

void ValidadeDialog::on_dateEdit_dateChanged(const QDate date) { ui->spinBoxDias->setValue(static_cast<int>(qApp->serverDate().daysTo(date))); }

int ValidadeDialog::getValidade() { return ui->checkBoxSemValidade->isChecked() ? -1 : ui->spinBoxDias->value(); }

void ValidadeDialog::on_checkBoxSemValidade_toggled(const bool checked) {
  ui->dateEdit->setEnabled(not checked);
  ui->spinBoxDias->setEnabled(not checked);
}
