
#ifndef CANRAWVIEWBACKEND_H
#define CANRAWVIEWBACKEND_H

#include <Qt> // SortOrder

#include "uibackend.h" // UIBackend, UIBackendDefault

#include <functional> // function


class QAbstractItemModel;
class CanRawView;
class CanRawViewPrivate;
class QWidget;

namespace Ui { class CanRawViewPrivate; }


template<>
class UIBackendDefault<CanRawView> : public UIBackend<CanRawView>
{

 public:

    CanRawViewBackend();

    virtual ~CanRawViewBackend() = default;

    virtual void setClearCbk(std::function<void ()> cb) override;

    virtual void setDockUndockCbk(std::function<void ()> cb) override;

    virtual void setSectionClikedCbk(std::function<void (int)> cb) override;

    virtual QWidget* getMainWidget() override;

    virtual void initTableView(QAbstractItemModel& tvModel) override;

    virtual void updateScroll() override;

    virtual int getSortOrder() override;

    virtual QString getClickedColumn(int ndx) override;

    virtual void setSorting(int sortNdx, int clickedNdx, Qt::SortOrder order) override;

 private:

    Ui::CanRawViewPrivate* ui;
    QWidget* widget;
};


using CanRawViewBackend = UIBackendDefault<CanRawView>;


#endif // CANRAWVIEWBACKEND_H
