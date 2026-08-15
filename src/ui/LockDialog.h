#pragma once

#include <QDialog>

class QLineEdit;

class LockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LockDialog(QWidget *parent = nullptr);

private slots:
    void onUnlock();
    void onTouchId();

private:
    QLineEdit *m_passwordEdit = nullptr;
};
