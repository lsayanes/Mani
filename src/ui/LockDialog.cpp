#include "ui/LockDialog.h"

#include "platform/TouchIdAuth.h"
#include "util/AppLock.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

LockDialog::LockDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Mani bloqueada"));
    setModal(true);
    resize(360, 180);

    auto *title = new QLabel(tr("Ingresa tu contraseña para continuar"), this);
    title->setWordWrap(true);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Contraseña"));

    auto *unlockButton = new QPushButton(tr("Desbloquear"), this);
    connect(unlockButton, &QPushButton::clicked, this, &LockDialog::onUnlock);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LockDialog::onUnlock);

    auto *touchIdButton = new QPushButton(tr("Desbloquear con Touch ID"), this);
    touchIdButton->setVisible(touchIdAvailable());
    connect(touchIdButton, &QPushButton::clicked, this, &LockDialog::onTouchId);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &LockDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(m_passwordEdit);
    layout->addWidget(unlockButton);
    if (touchIdAvailable()) {
        layout->addWidget(touchIdButton);
    }
    layout->addWidget(buttons);
}

void LockDialog::onUnlock()
{
    if (AppLock::verifyPassword(m_passwordEdit->text())) {
        accept();
        return;
    }

    QMessageBox::warning(this, tr("Contraseña incorrecta"), tr("Intenta de nuevo."));
    m_passwordEdit->clear();
    m_passwordEdit->setFocus();
}

void LockDialog::onTouchId()
{
    if (touchIdAuthenticate("Desbloquear Mani")) {
        accept();
        return;
    }

    QMessageBox::information(this, tr("Touch ID"),
                             tr("No se pudo desbloquear con biometria. Usa tu contraseña."));
}
