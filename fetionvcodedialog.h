#ifndef FETIONVCODEDIALOG_H
#define FETIONVCODEDIALOG_H

#include <QDialog>
class QImage;
class QLabel;
class QPushButton;
class KLineEdit;

class FetionVCodeDialog : public QDialog
{
    Q_OBJECT
    public:
        virtual ~FetionVCodeDialog();
        static QString getInputCode( const QImage& image, QWidget* parent = 0 );
    protected:
        explicit FetionVCodeDialog( const QImage& image, QWidget* parent = 0 );
        QString textValue() const;
    private:
        QLabel* m_imagelabel;
        KLineEdit* m_edit;
        QPushButton* m_confirm;
};

#endif // FETIONVCODEDIALOG_H
