#pragma once

#include <Tempest/Widget>
#include <Tempest/Sprite>
#include <Tempest/Window>
#include <Tempest/Icon>
#include <Tempest/Button>
#include <Tempest/Panel>
#include <Tempest/Signal>
#include <Tempest/UiOverlay>

namespace Tempest {

class Menu {
  private:
    struct Delegate;

  protected:
    struct Item;

  public:
    struct Declarator;

    Menu(Declarator&& decl);
    virtual ~Menu();

    void setMinimumWidth(int w);

    int  exec(Widget& owner);
    int  exec(Widget& owner, const Point &pos, bool alignWidthToOwner = true);
    void close();

    struct Declarator final {
      Declarator() = default;
      Declarator(Declarator&&) = default;
      Declarator& operator = (Declarator&&)=default;

      template<class Str, class ... Args>
      Declarator& item(const Str& text, Args... a){
        items.emplace_back();

        Item& it = items.back();
        assign(it.text,text);
        bind(it,a...);

        return *this;
        }

      template<class Str, class ... Args>
      Declarator& item(const Tempest::Icon& icon, const Str& text, Args... a){
        items.emplace_back();

        Item& it = items.back();
        assign(it.text,text);
        it.icon = icon;
        bind(it,a...);

        return *this;
        }

      template<class Str, class Str2, class ... Args>
      Declarator& item(const Tempest::Icon& icon, const Str& text, const Str2& text2, Args... a){
        items.emplace_back();

        Item& it = items.back();
        assign(it.text, text);
        assign(it.text2,text2);
        it.icon = icon;
        bind(it,a...);

        return *this;
        }

      Declarator& operator [](Declarator&& d){
        items.back().items = std::move(d);
        return *this;
        }

      Declarator& operator [](Declarator& d){
        items.back().items = std::move(d);
        return *this;
        }

      private:
        std::vector<Item> items;
        size_t            level = 0;

        void bind(Item&){}

        template<class ... Args>
        void bind(Item& it, Args... a){
          it.activated.bind(a...);
          }

      friend class Menu;
      };

  protected:
    struct Item {
      Item() = default;
      Item(Item&&) = default;
      Item& operator = (Item&&)=default;

      std::string             text, text2;
      Tempest::Icon           icon;
      Tempest::Signal<void()> activated;
      Declarator              items;
      };

    virtual Widget* createDropList(Widget &owner,
                                   bool alignWidthToOwner,
                                   const std::vector<Item> &items);
    virtual Widget* createItems   (const std::vector<Item> &items);
    virtual Widget* createItem    (const Item& decl);

    struct MenuPanel:Tempest::Panel {
      void paintEvent(Tempest::PaintEvent& e) override;

      void setSize(const Size& s);
      };

    class ItemButton:public Tempest::Button {
      public:
        ItemButton(const Declarator& item);

        void mouseEnterEvent(MouseEvent& e) override;
        void paintEvent(Tempest::PaintEvent& e) override;

        void setExtraText(const char*        text);
        void setExtraText(const std::string& text);

        void setExtraFont(const Font& f);

        Tempest::Signal<void(const Declarator&,Widget&)> onMouseEnter;

      private:
        const Declarator& item;
        TextModel         text2;
      };

    void openSubMenu(const Declarator &items, Widget& root);
    void setupMenuPanel(Widget& box);

  private:
    struct Overlay:Tempest::UiOverlay {
      Overlay(Menu* menu, Widget* owner);
      ~Overlay() override;
      void mouseDownEvent(Tempest::MouseEvent& e) override;
      void mouseMoveEvent(Tempest::MouseEvent& e) override;
      void resizeEvent   (Tempest::SizeEvent&  e) override;

      void invalidatePos();

      Widget* owner;
      Menu  * menu;
      };

    struct MPanel {
      Widget*        widget = nullptr;
      Tempest::Point pos;
      };

    void initMenuLevels(Declarator& decl);

    std::vector<MPanel>  panels;
    Overlay*             overlay = nullptr;
    bool                 running = false;
    Declarator           decl;
    int                  minW = 0;

    static void assign(std::string& s, const char*     ch);
    static void assign(std::string& s, const char16_t* ch);

    static void assign(std::string& s, const std::string& ch);
    static void assign(std::string& s, const std::u16string& ch);
    void implClose();
  };
}

