#include "validadedialog.h"
#include "ui_validadedialog.h"

#include "application.h"

ValidadeDialog::ValidadeDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ValidadeDialog) {
  ui->setupUi(this);

  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &ValidadeDialog::on_pushButtonSalvar_clicked);
  connect(ui->spinBox, qOverload<int>(&QSpinBox::valueChanged), this, &ValidadeDialog::on_spinBox_valueChanged);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &ValidadeDialog::on_dateEdit_dateChanged);

  ui->dateEdit->setDate(qApp->serverDate());
}

ValidadeDialog::~ValidadeDialog() { delete ui; }

void ValidadeDialog::on_pushButtonSalvar_clicked() {
  QDialog::accept();
  close();
}

void ValidadeDialog::on_spinBox_valueChanged(const int dias) { ui->dateEdit->setDate(qApp->serverDate().addDays(dias)); }

void ValidadeDialog::on_dateEdit_dateChanged(const QDate &date) { ui->spinBox->setValue(static_cast<int>(qApp->serverDate().daysTo(date))); }

int ValidadeDialog::getValidade() { return ui->spinBox->value(); }
