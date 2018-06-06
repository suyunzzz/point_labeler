#include "Mainframe.h"

#include <fstream>
#include <iostream>
#include <map>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QFileDialog>

#include "../data/label_utils.h"
#include "../data/misc.h"

#include <QtWidgets/QMessageBox>

using namespace glow;

Mainframe::Mainframe() : mChangesSinceLastSave(false) {
  ui.setupUi(this);

  connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(open()));
  connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));

  /** initialize the paint button mapping **/
  connect(ui.actionPaintMode, &QAction::triggered, [this]() { changeMode(Viewport::PAINT); });

  connect(ui.mViewportXYZ, SIGNAL(labelingChanged()), this, SLOT(unsavedChanges()));

  connect(ui.btnOverwrite, SIGNAL(toggled(bool)), ui.mViewportXYZ, SLOT(setOverwrite(bool)));

  connect(ui.spinPointSize, SIGNAL(valueChanged(int)), ui.mViewportXYZ, SLOT(setPointSize(int)));

  connect(ui.btnRadius5, &QToolButton::released, [this]() { changeRadius(5); });
  connect(ui.btnRadius10, &QToolButton::released, [this]() { changeRadius(10); });
  connect(ui.btnRadius20, &QToolButton::released, [this]() { changeRadius(20); });

  connect(ui.mRadiusSlider, SIGNAL(valueChanged(int)), this, SLOT(changeRadius(int)));
  connect(ui.sldTimeline, &QSlider::valueChanged, [this](int value) { setCurrentScanIdx(value); });

  /** load labels and colors **/
  std::map<uint32_t, std::string> label_names;
  std::map<uint32_t, glow::GlColor> label_colors;

  getLabelNames("labels.xml", label_names);
  getLabelColors("labels.xml", label_colors);

  ui.mViewportXYZ->setLabelColors(label_colors);

  generateLabelButtons();
}

Mainframe::~Mainframe() {}

void Mainframe::closeEvent(QCloseEvent* event) {
  //  if (mChangesSinceLastSave) {
  //    int ret = QMessageBox::warning(this, tr("Unsaved changes."), tr("The annotation has been modified.\n"
  //                                                                    "Do you want to save your changes?"),
  //                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
  //                                   QMessageBox::Save);
  //    if (ret == QMessageBox::Save)
  //      save();
  //    else if (ret == QMessageBox::Cancel) {
  //      event->ignore();
  //      return;
  //    }
  //  }

  event->accept();
}

void Mainframe::open() {
  //  if (mChangesSinceLastSave) {
  //    int ret = QMessageBox::warning(this, tr("Unsaved changes."), tr("The annotation has been modified.\n"
  //                                                                    "Do you want to save your changes?"),
  //                                   QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard,
  //                                   QMessageBox::Save);
  //    if (ret == QMessageBox::Save)
  //      save();
  //    else if (ret == QMessageBox::Cancel)
  //      return;
  //  }

  QString retValue =
      QFileDialog::getExistingDirectory(this, "Select scan directory", lastDirectory, QFileDialog::ShowDirsOnly);

  if (!retValue.isNull()) {
    QDir base_dir(retValue);

    if (!base_dir.exists("velodyne") || !base_dir.exists("poses.txt")) {
      std::cout << "[ERROR] velodyne or poses.txt missing." << std::endl;
      return;
    }

    reader_.initialize(retValue);

    ui.sldTimeline->setMaximum(reader_.count());

    if (ui.sldTimeline->value() == 0) setCurrentScanIdx(0);
    ui.sldTimeline->setValue(0);

    lastDirectory = base_dir.absolutePath();

    changeMode(Viewport::NONE);

    QString title = "Point Labeler - ";
    title += QFileInfo(retValue).completeBaseName();
    setWindowTitle(title);

    mChangesSinceLastSave = false;
  }
}

void Mainframe::save() {
  // TODO: write binary format and accombining header.
  //  QFileInfo info(QString::fromStdString(filename));
  //  QString labelName = info.absolutePath() + "/" + info.baseName();
  //
  //  std::string labels_file = labelName.toStdString();
  //  labels_file += ".label.xyz";
  //
  //  /* (3) overwrite the complete label file with new label information. */
  //  std::ofstream out(labels_file.c_str());
  //  for (uint32_t i = 0; i < points.size() - 1; ++i) out << labels[i] << std::endl;
  //  out << labels[labels.size() - 1];
  //  out.close();

  mChangesSinceLastSave = false;
}

void Mainframe::changeRadius(int value) {
  ui.btnRadius5->setChecked(false);
  ui.btnRadius10->setChecked(false);
  ui.btnRadius20->setChecked(false);

  switch (value) {
    case 5:
      ui.btnRadius5->setChecked(true);
      break;
    case 10:
      ui.btnRadius10->setChecked(true);
      break;
    case 20:
      ui.btnRadius20->setChecked(true);
      break;
  }

  ui.mRadiusSlider->setValue(value);

  ui.mViewportXYZ->setRadius(value);
}

void Mainframe::changeMode(int mode) {
  ui.mViewportXYZ->setMode(Viewport::NONE);

  if (mode == Viewport::PAINT && ui.actionPaintMode->isChecked()) /** PAINTMODE **/
  {
    ui.mViewportXYZ->setMode(Viewport::PAINT);

    ui.actionCylinderMode->setChecked(false);

    ui.mTools->setCurrentIndex(1);
  } else if (mode == Viewport::NONE) {
    ui.actionPaintMode->setChecked(false);
    ui.actionCylinderMode->setChecked(false);
  }
}

// void Mainframe::readXYZ(const std::string& filename) {
//  std::ifstream in(filename.c_str());
//  if (!in.is_open()) throw "unable to open XYZ file.";
//
//  QFileInfo info(QString::fromStdString(filename));
//  QString labelName = info.absolutePath() + "/" + info.baseName();
//  std::string labels_file = labelName.toStdString();
//
//  labels_file += ".label.xyz";
//
//  //  std::string cylinder_file = RoSe::FileUtil::stripExtension(filename, 1);
//  //  cylinder_file += ".cylinder.xyz";
//
//  std::cout << "labels-filename: " << labels_file << std::endl;
//  //  std::cout << "cylinder_filename: " << cylinder_file << std::endl;
//
//  std::ifstream in_labels(labels_file.c_str());
//  std::ofstream out_labels;
//
//  bool generate_labels = false;
//  if (!in_labels.is_open()) {
//    std::cout << "labels file not found. generating labels file." << std::endl;
//    generate_labels = true;
//    out_labels.open(labels_file.c_str());
//    if (!out_labels.is_open()) throw "unable to generate label filename.";
//  }
//
//  //  std::ifstream in_cylinders(cylinder_file.c_str());
//
//  std::string line;
//  //  Point3f midpoint;
//
//  in.peek();
//  int idx = 0;
//  while (!in.eof() && in.good()) {
//    std::getline(in, line);
//    std::vector<std::string> tokens = split(line, ";");
//
//    if (tokens.size() < 3) continue;
//
//    float x = QString::fromStdString(tokens[0]).toFloat();
//    float y = QString::fromStdString(tokens[1]).toFloat();
//    float z = QString::fromStdString(tokens[2]).toFloat();
//
//    if (!generate_labels) {
//      std::getline(in_labels, line);
//      labels.push_back(QString::fromStdString(line).toInt());
//    } else {
//      out_labels << "0" << std::endl;
//      labels.push_back(0);
//    }
//
//    points.push_back(Point3f(x, y, z));
//    midpoint += points.back();
//
//    in.peek();
//    ++idx;
//  }
//
//  midpoint /= points.size();
//
//  /** substract the midpoint **/
//  for (uint32_t i = 0; i < points.size(); ++i) {
//    points[i] = Point3f(points[i] - midpoint);
//  }
//
//  in.close();
//  in_labels.close();
//  //  in_cylinders.close();
//  out_labels.close();
//}

void Mainframe::generateLabelButtons() {
  const int BtnsPerRow = 5;

  std::map<uint32_t, std::string> label_names;
  std::map<uint32_t, GlColor> label_colors;

  getLabelNames("labels.xml", label_names);
  getLabelColors("labels.xml", label_colors);

  labelButtonMapper = new QSignalMapper(this);

  uint32_t index = 0;

  QGridLayout* layout = dynamic_cast<QGridLayout*>(ui.labelsGroupBox->layout());
  if (layout == 0) throw "Layout of classesGroupBox is not a QGridLayout!";
  ui.labelsGroupBox->setLayout(layout);

  std::map<uint32_t, std::string>::const_iterator namesIter = label_names.begin();
  std::map<uint32_t, GlColor>::const_iterator colorsIter = label_colors.begin();

  for (; namesIter != label_names.end(); ++namesIter, ++colorsIter, ++index) {
    const int id = namesIter->first;
    const std::string& name = namesIter->second;
    const GlColor& color = colorsIter->second;

    LabelButton* newButton = new LabelButton(ui.labelsGroupBox, QColor(color.R * 255, color.G * 255, color.B * 255));
    newButton->setAutoFillBackground(true);
    labelButtons.push_back(newButton);
    labelIds[newButton] = id;
    layout->addWidget(newButton, std::floor((double)index / BtnsPerRow),
                      index - std::floor((double)index / BtnsPerRow) * BtnsPerRow);

    newButton->setCheckable(true);
    newButton->setFixedSize(30, 30);

    newButton->setStatusTip(QString::fromStdString(name));

    /* connect the button with mapper which dispatches a signal with the index of the clicked button */
    labelButtonMapper->setMapping(newButton, newButton);
    connect(newButton, SIGNAL(released()), labelButtonMapper, SLOT(map()));

    QColor col(255.0f * color.R, 255.0f * color.G, 255.0f * color.B, 255);
    QPixmap pix(10, 10);
    pix.fill(col);
    QIcon icon(pix);

    idxLabelMap[index] = id;
    labelIdxMap[id] = index;
  }
  /** register only once the signal mapped to labelBtnReleased! **/
  connect(labelButtonMapper, SIGNAL(mapped(QWidget*)), this, SLOT(labelBtnReleased(QWidget*)));

  if (labelButtons.size() > 0) labelBtnReleased(labelButtons[0]);
}

void Mainframe::updateFiltering(bool value) {
  if (value) {
    ui.mViewportXYZ->setFilteredLabels(filteredLabels);
  } else {
    std::vector<uint32_t> empty;

    ui.mViewportXYZ->setFilteredLabels(empty);
  }
}

void Mainframe::labelBtnReleased(QWidget* w) {
  //  std::cout << "labelBtnReleased called." << std::endl;
  LabelButton* labelButton = dynamic_cast<LabelButton*>(w);
  if (labelButton == 0) return;
  if (labelIds.find(labelButton) == labelIds.end()) return;

  for (unsigned int i = 0; i < labelButtons.size(); ++i) labelButtons[i]->setChecked(false);

  labelButton->setChecked(true);
  uint32_t label_id = labelIds[labelButton];

  ui.mViewportXYZ->setLabel(label_id);

  if (labelButton->isHighlighted()) {
    filteredLabels.push_back(label_id);
    updateFiltering(ui.chkFilterLabels->isChecked());
  } else {
    std::vector<uint32_t> tempFilteredLabels;

    for (uint32_t i = 0; i < filteredLabels.size(); ++i)
      if (filteredLabels[i] != label_id) tempFilteredLabels.push_back(filteredLabels[i]);

    filteredLabels = tempFilteredLabels;
    updateFiltering(ui.chkFilterLabels->isChecked());
  }
}

void Mainframe::unsavedChanges() {
  mChangesSinceLastSave = true;
}

void Mainframe::setCurrentScanIdx(int32_t idx) {
  std::cout << "setting scan." << std::endl;

  reader_.update(indexes_, labels_);

  reader_.retrieve(idx, indexes_, points_, labels_);

  ui.mViewportXYZ->setPoints(points_, labels_);
}