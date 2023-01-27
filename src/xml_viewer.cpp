#include "xml_viewer.h"
#include "ui_xml_viewer.h"

#include "acbrlib.h"

XML_Viewer::XML_Viewer(const QString &content, QWidget *parent) : QDialog(parent), xml(content), ui(new Ui::XML_Viewer) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  ui->treeView->setModel(&xml.model);
  ui->treeView->expandAll();

  setConnections();

  show();
}

XML_Viewer::~XML_Viewer() { delete ui; }

void XML_Viewer::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &XML_Viewer::on_pushButtonDanfe_clicked, connectionType);
}

void XML_Viewer::on_pushButtonDanfe_clicked() {
#ifdef Q_OS_WIN
  ACBrLib::gerarDanfe(xml.fileContent, true);
#else
  throw RuntimeException("Não implementado para linux!");
#endif
}
