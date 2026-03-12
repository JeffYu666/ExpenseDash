#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QSqlRelationalTableModel>
#include "user.h"

namespace Ui { class SearchDialog; }

class SearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit SearchDialog(QSqlRelationalTableModel *model,
                          const User *currentUser,
                          QWidget *parent = nullptr);
    ~SearchDialog();

public slots:
    void onButtonBoxAccepted();
    void onButtonBoxRejected();

private:
    Ui::SearchDialog *ui;
    QSqlRelationalTableModel *m_model = nullptr;
    const User *m_currentUser = nullptr;

    void populateCategoryComboBox();
    void setupDateRange();
};

#endif // SEARCHDIALOG_H
