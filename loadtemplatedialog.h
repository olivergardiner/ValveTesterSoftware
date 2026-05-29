#pragma once
#include <QDialog>
#include <QString>
#include "templatestore.h"

class QListWidget;

class LoadTemplateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoadTemplateDialog(const TemplateStore &store, QWidget *parent = nullptr);

    // Returns the selected template name, or empty string if none selected
    QString selectedName() const;

private:
    QListWidget *m_list;
};
