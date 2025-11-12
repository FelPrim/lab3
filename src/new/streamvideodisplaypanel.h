#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "../videodisplay.h"

class StreamVideoDisplayPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StreamVideoDisplayPanel(QWidget *parent = nullptr);
    ~StreamVideoDisplayPanel();

    void setPlaceholderText(const QString &text);
    void showVideo(bool show);
    void setStreamId(int streamId);

public slots:
    void displayFrame(const QImage &frame);
    void clearDisplay();

private:
    void setupUI();
    void clearPlaceholder();

    VideoDisplay *m_videoDisplayWidget; // делегируем отображение
    QLabel *m_placeholderLabel;
    QVBoxLayout *m_layout;
};
