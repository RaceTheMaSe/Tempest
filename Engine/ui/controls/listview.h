#pragma once

#include <Tempest/Widget>
#include <Tempest/ScrollWidget>
#include <Tempest/ListDelegate>
#include <Tempest/Layout>

namespace Tempest {

class ListView : public Widget {
  public:
    ListView(Orientation ori=Vertical);
    ~ListView() override;

    Tempest::Signal<void(size_t)> onItemSelected;
    Tempest::Signal<void()>       onItemListChanged;

    Widget& centralWidget();

    template<class T>
    auto setDelegate(T* d) -> T* {
      implSetDelegate(d);
      return d;
      }
    void removeDelegate();

    void setLayout(Tempest::Orientation ori);

    void setDefaultItemRole(ListDelegate::Role role);
    auto defaultItemRole() const -> ListDelegate::Role { return defaultRole; }

    void invalidateView();
    void updateView();

  private:
    void implSetDelegate(ListDelegate* d);

    ScrollWidget                   sc;
    std::unique_ptr<ListDelegate>  delegate;
    ListDelegate::Role             defaultRole = ListDelegate::R_ListItem;
  };

}

