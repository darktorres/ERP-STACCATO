#include "xml_viewer.h"
#include "acbr.h"
#include "ui_xml_viewer.h"
#include "xml.h"

XML_Viewer::XML_Viewer(QWidget *parent) : Dialog(parent), ui(new Ui::XML_Viewer) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  ui->treeView->setModel(&model);
  ui->treeView->setUniformRowHeights(true);
  ui->treeView->setAnimated(true);
  ui->treeView->setEditTriggers(QTreeView::NoEditTriggers);

  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &XML_Viewer::on_pushButtonDanfe_clicked);
}

XML_Viewer::~XML_Viewer() { delete ui; }

void XML_Viewer::exibirXML(const QByteArray &content) {
  if (content.isEmpty()) return;

  fileContent = content;

  XML xml(content);
  xml.montarArvore(model);

  ui->treeView->expandAll();

  show();
}

void XML_Viewer::on_pushButtonDanfe_clicked() { ACBr::gerarDanfe(fileContent); }
