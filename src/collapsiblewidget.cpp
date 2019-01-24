#include "collapsiblewidget.h"
#include "ui_collapsiblewidget.h"

#include <qevent.h>

CollapsibleWidget::CollapsibleWidget(const QString &data, QWidget *parent) : QWidget(parent), ui(new Ui::CollapsibleWidget) {
  ui->setupUi(this);

  connect(ui->pushButton, &QPushButton::clicked, this, &CollapsibleWidget::on_pushButton_clicked);

  //  ui->textBrowser->setLineWrapMode(QTextBrowser::NoWrap);
  //  ui->textBrowser->setHidden(true);
  ui->pushButton_2->setHidden(true);

  ui->lineEdit->setText(data);

  QFont font("", 0);
  QFontMetrics fm(font);
  int pixelsWide = fm.width(data);
  int pixelsHigh = fm.height();

  ui->lineEdit->setFixedSize(pixelsWide, pixelsHigh);

  //  adjustSize();

  //  setMinimumSize(300, 300);

  // void CollapsibleWidget::setHtml(const QString text) { ui->textBrowser->setHtml(text); }

  // QString CollapsibleWidget::getHtml() {
  //  if (ui->textBrowser->toPlainText().isEmpty()) { return QString(); }

  //  return ui->textBrowser->toHtml();
}

void CollapsibleWidget::setDestinos(const QString &value) { destinos = value; }

void CollapsibleWidget::addButton() { layout()->addWidget(new QPushButton(this)); }

CollapsibleWidget::~CollapsibleWidget() { delete ui; }

void CollapsibleWidget::on_pushButton_clicked() {
  //  ui->textBrowser->setVisible(not ui->textBrowser->isVisible());
  //  ui->pushButton_2->setVisible(not ui->pushButton_2->isVisible());
  //  ui->pushButton->setText(ui->textBrowser->isVisible() ? "↑" : "↓");
  //  emit toggled();
}

void CollapsibleWidget::resizeEvent(QResizeEvent *event) {
  emit resized(event->size());
  event->accept();
}
