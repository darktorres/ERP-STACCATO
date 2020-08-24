#include "xml_viewer.h"
#include "ui_xml_viewer.h"

#include "acbr.h"

XML_Viewer::XML_Viewer(const QByteArray &content, QWidget *parent) : QDialog(parent), fileContent(content), xml(content), ui(new Ui::XML_Viewer) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  ui->treeView->setModel(&xml.model);
  ui->treeView->setUniformRowHeights(true);
  ui->treeView->setAnimated(true);
  ui->treeView->setEditTriggers(QTreeView::NoEditTriggers);
  ui->treeView->expandAll();

  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &XML_Viewer::on_pushButtonDanfe_clicked);

  show();
}

XML_Viewer::~XML_Viewer() { delete ui; }

void XML_Viewer::on_pushButtonDanfe_clicked() {
  ACBr acbrLocal(this);
  acbrLocal.gerarDanfe(fileContent);
}
