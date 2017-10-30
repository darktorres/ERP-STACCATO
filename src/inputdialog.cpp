#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "doubledelegate.h"
#include "inputdialog.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "singleeditdelegate.h"
#include "ui_inputdialog.h"
#include "usersession.h"

InputDialog::InputDialog(const Tipo &tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialog) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  ui->dateEditEvento->setDate(QDate::currentDate());
  ui->dateEditProximo->setDate(QDate::currentDate());

  if (tipo == Tipo::Carrinho) {
    ui->labelEvento->hide();
    ui->dateEditEvento->hide();
    ui->labelObservacao->hide();
    ui->lineEditObservacao->hide();

    ui->labelProximoEvento->setText("Data prevista compra:");
  }

  if (tipo == Tipo::Faturamento) {
    ui->labelEvento->hide();
    ui->dateEditEvento->hide();
    ui->labelObservacao->hide();
    ui->lineEditObservacao->hide();

    ui->labelProximoEvento->setText("Data prevista faturamento:");
  }

  if (tipo == Tipo::AgendarColeta) {
    ui->labelEvento->hide();
    ui->dateEditEvento->hide();
    ui->labelProximoEvento->show();
    ui->dateEditProximo->show();
    ui->labelObservacao->hide();
    ui->lineEditObservacao->hide();

    ui->labelProximoEvento->setText("Data prevista coleta");
    ui->dateEditProximo->setDate(QDate::currentDate().addDays(8));
  }

  if (tipo == Tipo::Coleta) {
    ui->labelObservacao->hide();
    ui->lineEditObservacao->hide();

    ui->labelEvento->setText("Data coleta:");
    ui->labelProximoEvento->setText("Data prevista recebimento:");
  }

  if (tipo == Tipo::AgendarRecebimento) {
    ui->labelObservacao->hide();
    ui->lineEditObservacao->hide();
    ui->labelEvento->hide();
    ui->dateEditEvento->hide();
    ui->labelProximoEvento->show();
    ui->dateEditProximo->show();

    ui->labelProximoEvento->setText("Data prevista recebimento:");
  }

  if (tipo == Tipo::AgendarEntrega) {
    ui->labelEvento->hide();
    ui->dateEditEvento->hide();
    ui->labelProximoEvento->show();
    ui->dateEditProximo->show();
    ui->labelObservacao->hide();
    ui->lineEditObservacao->hide();

    ui->labelProximoEvento->setText("Data prevista entrega:");
  }

  if (tipo == Tipo::ReagendarPedido) {
    ui->labelEvento->hide();
    ui->dateEditEvento->hide();
    ui->labelProximoEvento->show();
    ui->dateEditProximo->show();
    ui->labelObservacao->show();
    ui->lineEditObservacao->show();

    ui->labelProximoEvento->setText("Data prevista:");
  }

  adjustSize();

  show();
}

InputDialog::~InputDialog() { delete ui; }

QDate InputDialog::getDate() const { return ui->dateEditEvento->date(); }

QDate InputDialog::getNextDate() const { return ui->dateEditProximo->date(); }

QString InputDialog::getObservacao() const { return ui->lineEditObservacao->text(); }

void InputDialog::on_dateEditEvento_dateChanged(const QDate &date) {
  if (ui->dateEditProximo->date() < date) ui->dateEditProximo->setDate(date);
}

void InputDialog::on_pushButtonSalvar_clicked() {
  if (tipo == Tipo::ReagendarPedido) {
    if (ui->lineEditObservacao->text().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Observação não pode estar vazio!");
      return;
    }
  }

  QDialog::accept();
  close();
}

// TODO: 0colocar titulo, descricao para nao perder o fluxo
// REFAC: colocar as classes variacoes (Produto/Financeiro) como widget e colocar nesta classe
