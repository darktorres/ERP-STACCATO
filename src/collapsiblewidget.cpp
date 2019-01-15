#include "collapsiblewidget.h"
#include "ui_collapsiblewidget.h"

CollapsibleWidget::CollapsibleWidget(QWidget *parent) : QWidget(parent), ui(new Ui::CollapsibleWidget) {
  ui->setupUi(this);

  connect(ui->pushButton, &QPushButton::clicked, this, &CollapsibleWidget::on_pushButton_clicked);

  ui->textBrowser->setLineWrapMode(QTextBrowser::NoWrap);
  ui->textBrowser->setHidden(true);
}

void CollapsibleWidget::setHtml(const QString text) { ui->textBrowser->setHtml(text); }

QString CollapsibleWidget::getHtml() {
  if (ui->textBrowser->toPlainText().isEmpty()) { return QString(); }

  return ui->textBrowser->toHtml();
}

CollapsibleWidget::~CollapsibleWidget() { delete ui; }

void CollapsibleWidget::on_pushButton_clicked() {
  ui->textBrowser->setVisible(not ui->textBrowser->isVisible());
  ui->pushButton->setText(ui->textBrowser->isVisible() ? "Esconder" : "Mostrar");
  emit toggled();
}
