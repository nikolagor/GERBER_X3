/*******************************************************************************
* Author    :  Damir Bakiev                                                    *
* Version   :  na                                                              *
* Date      :  11 November 2021                                                *
* Website   :  na                                                              *
* Copyright :  Damir Bakiev 2016-2022                                          *
* License:                                                                     *
* Use, modification & distribution is subject to Boost Software License Ver 1. *
* http://www.boost.org/LICENSE_1_0.txt                                         *
*******************************************************************************/
#pragma once

#include <QDialog>

class QDialogButtonBox;
class QPushButton;
class QTreeWidget;
class QVBoxLayout;

class DialogAboutPlugins : public QDialog {
    QDialogButtonBox* buttonBox;
    QTreeWidget* treeWidget;
    QVBoxLayout* verticalLayout;
    QPushButton* pbClose;
    QPushButton* pbInfo;

    void setupUi(QDialog* Dialog); // setupUi

    void retranslateUi(QDialog* Dialog); // retranslateUi

public:
    DialogAboutPlugins(QWidget* parent = nullptr);
    virtual ~DialogAboutPlugins();
};
