#include "validadedialog.h"
#include "ui_validadedialog.h"

ValidadeDialog::ValidadeDialog(QWidget *parent) : Dialog(parent), ui(new Ui::ValidadeDialog) {
  ui->setupUi(this);

  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &ValidadeDialog::on_pushButtonSalvar_clicked);
  connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ValidadeDialog::on_spinBox_valueChanged);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &ValidadeDialog::on_dateEdit_dateChanged);

  ui->dateEdit->setDate(QDate::currentDate());
}

ValidadeDialog::~ValidadeDialog() { delete ui; }

void ValidadeDialog::on_pushButtonSalvar_clicked() {
  QDialog::accept();
  close();
}

void ValidadeDialog::on_spinBox_valueChanged(const int dias) { ui->dateEdit->setDate(QDate::currentDate().addDays(dias)); }

void ValidadeDialog::on_dateEdit_dateChanged(const QDate &date) { ui->spinBox->setValue(static_cast<int>(QDate::currentDate().daysTo(date))); }

int ValidadeDialog::getValidade() { return ui->spinBox->value(); }
