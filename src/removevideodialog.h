#pragma once

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVector>
#include "videocapture.h"  

class RemoveVideoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RemoveVideoDialog(const QVector<VideoCapture*>& videoCaptures, QWidget *parent = nullptr); 
    int selectedIndex() const { return m_selectedIndex; }

private slots:
    void onItemSelected(int index);

private:
    QListWidget *m_listWidget;
    int m_selectedIndex = -1;
};