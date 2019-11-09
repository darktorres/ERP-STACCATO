#include "validadedialog.h"
#include "application.h"
#include "ui_validadedialog.h"

ValidadeDialog::ValidadeDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ValidadeDialog) {
  ui->setupUi(this);

  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &ValidadeDialog::on_pushButtonSalvar_clicked);
  connect(ui->spinBox, qOverload<int>(&QSpinBox::valueChanged), this, &ValidadeDialog::on_spinBox_valueChanged);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &ValidadeDialog::on_dateEdit_dateChanged);

  ui->dateEdit->setDate(qApp->serverDateTime().date());
}

ValidadeDialog::~ValidadeDialog() { delete ui; }

void ValidadeDialog::on_pushButtonSalvar_clicked() {
  QDialog::accept();
  close();
}

void ValidadeDialog::on_spinBox_valueChanged(const int dias) { ui->dateEdit->setDate(qApp->serverDateTime().date().addDays(dias)); }

void ValidadeDialog::on_dateEdit_dateChanged(const QDate &date) { ui->spinBox->setValue(static_cast<int>(qApp->serverDateTime().date().daysTo(date))); }

int ValidadeDialog::getValidade() { return ui->spinBox->value(); }
