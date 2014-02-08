#ifndef CUSTOMIZESPLASHDIALOG_H
#define CUSTOMIZESPLASHDIALOG_H

#include <QtGui>
#include <QDialog>
#include "flashinterface.h"

namespace Ui
{
  class customizeSplashDialog;
}

enum Source {FW, PICT, PROFILE, UNDEFINED};

class Side
{
public:
  Side();
  void copyImage( Side );

  QLabel *imageLabel;
  QLineEdit *fileNameEdit;
  QPushButton *saveButton;
  QPushButton *invertButton;
  QToolButton *libraryButton;

  Source *source;
};

class customizeSplashDialog : public QDialog
{
  Q_OBJECT
  Side left;
  Side right;

public:
  explicit customizeSplashDialog(QWidget *parent = 0);
  ~customizeSplashDialog();

private slots:
  void on_leftLoadFwButton_clicked();
  void on_leftLoadPictButton_clicked();
  void on_leftLoadProfileButton_clicked();
  void on_leftLibraryButton_clicked();
  void on_leftSaveButton_clicked();
  void on_leftInvertButton_clicked();

  void on_rightLoadFwButton_clicked();
  void on_rightLoadPictButton_clicked();
  void on_rightLoadProfileButton_clicked();
  void on_rightLibraryButton_clicked();
  void on_rightSaveButton_clicked();
  void on_rightInvertButton_clicked();

  void on_copyRightToLeftButton_clicked();
  void on_copyLeftToRightButton_clicked();

private:
  void loadFirmware( Side );
  void loadPicture( Side );
  void loadProfile( Side );
  void libraryButton_clicked( Side );
  void saveButton_clicked( Side );
  void invertButton_clicked( Side );

  Ui::customizeSplashDialog *ui;
  enum sideEnum { LEFT, RIGHT };
};

#endif // CUSTOMIZESPLASHDIALOG_H
