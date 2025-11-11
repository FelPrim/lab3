#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRegularExpressionValidator>

class StreamIdInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StreamIdInputWidget(QWidget *parent = nullptr);

    QString getStreamId() const;
    bool isValid() const;
    void clear();
    void setEnabled(bool enabled);
    void setStreamId(const QString &streamId);

signals:
    void streamIdChanged(const QString &streamId);
    void joinRequested(const QString &streamId);

private slots:
    void onTextChanged(const QString &text);
    void onJoinClicked();

private:
    void setupUI();
    void setupConnections();

    QLineEdit *m_streamIdInput;
    QPushButton *m_joinButton;
    QRegularExpressionValidator *m_validator;
    
    static const QRegularExpression STREAM_ID_REGEX;
};
