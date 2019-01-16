#include "xmlDistanceAPI.h"
#include "application.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

XML_Distance::XML_Distance(const QByteArray &fileContent, const int evento) : evento(evento), fileContent(fileContent) { montarArvore(); }

void XML_Distance::readChild(const QDomElement &element, QStandardItem *elementItem) {
  QDomElement child = element.firstChildElement();

  for (; not child.isNull(); child = child.nextSiblingElement()) {
    if (child.firstChild().isText()) {
      QStandardItem *childItem = new QStandardItem(child.nodeName() + " - " + child.text());
      elementItem->appendRow(childItem);
      continue;
    }

    QDomNamedNodeMap map = child.attributes();
    QString attributes = child.nodeName();

    if (map.size() > 0) {
      for (int i = 0; i < map.size(); ++i) { attributes += " " + map.item(i).nodeName() + R"(=")" + map.item(i).nodeValue() + R"(")"; }
    }

    auto *childItem = new QStandardItem(attributes);
    elementItem->appendRow(childItem);
    readChild(child, childItem);
  }
}

void XML_Distance::lerValores(const QStandardItem *item) {
  for (int row = 0; row < item->rowCount(); ++row) {
    for (int col = 0; col < item->columnCount(); ++col) {
      const QStandardItem *child = item->child(row, col);
      const QString parentText = child->parent()->text();
      QString text = child->text();

      //      qDebug() << "parent: " << parentText << " -> " << text;
      //      if (parentText == "distance") { qDebug() << text; }

      if (parentText == "distance" and text.left(5) == "value") {
        distancia = text.remove(0, 8);
        //        qDebug() << distancia;
      }

      if (parentText == "leg" and text.left(13) == "start_address") {
        startAddr = text.remove(0, 16).replace("'", "");
        //        qDebug() << startAddr;
      }
      if (parentText == "leg" and text.left(11) == "end_address") {
        endAddr = text.remove(0, 14).replace("'", "");
        //        qDebug() << endAddr;

        legs << QStringList{startAddr, endAddr, distancia};

        QSqlQuery query;

        if (not query.exec("INSERT INTO temp_distancia (evento, origem, destino, distancia) VALUES (" + QString::number(evento) + ", '" + startAddr + "', '" + endAddr + "', " + distancia + ")")) {
          qDebug() << "erro temp_distancia: " + query.lastError().text();
          qDebug() << "INSERT INTO temp_distancia (evento, origem, destino, distancia) VALUES (" + QString::number(evento) + ", '" + startAddr + "', '" + endAddr + "', " + distancia + ")";
        }
      }

      if (child->hasChildren()) { lerValores(child); }
    }
  }
}

void XML_Distance::montarArvore() {
  if (fileContent.isEmpty()) { return; }

  QDomDocument document;
  QString error;

  if (not document.setContent(fileContent, &error)) { return qApp->enqueueError("Erro lendo arquivo: " + error); }

  QDomElement root = document.firstChildElement();
  QDomNamedNodeMap map = root.attributes();
  QString attributes = root.nodeName();

  if (map.size() > 0) {
    for (int i = 0; i < map.size(); ++i) { attributes += " " + map.item(i).nodeName() + R"(=")" + map.item(i).nodeValue() + R"(")"; }
  }

  auto *rootItem = new QStandardItem(attributes);

  model.appendRow(rootItem);

  readChild(root, rootItem);

  lerValores(model.item(0, 0));
}
