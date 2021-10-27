#pragma once

#include <Tempest/Timer>
#include <Tempest/Rect>
#include <Tempest/TextModel>
#include <Tempest/Color>

#include <string>
#include <memory>
#include <cstdint>

namespace Tempest {

struct Margin;
class  Painter;
class  Font;
class  Color;
class  Icon;
class  Sprite;

class  TextModel;
class  WidgetState;

class  Application;
class  Widget;
class  Panel;
class  Dialog;

class  Button;
class  CheckBox;
class  Label;
class  ScrollBar;
class  ComboBox;
class  AbstractTextInput;

class Style {
  public:
    Style();
    virtual ~Style();
    Style(const Style& )=delete;
    Style& operator=(const Style&)=delete;

    struct UiMetrics final {
      UiMetrics();
      static int scaledSize(int x);

      int   buttonSize       = 27;
      int   scrollbarSize    = 27;
      int   scrollButtonSize = 40;

      int   smallTextSize    = 8;
      int   normalTextSize   = 16;
      int   largeTextSize    = 24;

      int   margin           = 8;
      };

    struct UiColors final {
      Color text = Color(1.f);
      };

    struct Extra {
      public:
        Extra(const Widget&            owner);
        Extra(const Button&            owner);
        Extra(const Label&             owner);
        Extra(const AbstractTextInput& owner);

        const Margin&         margin;
        const Icon&           icon;
        const Color&          fontColor;
        int                   spacing=0;

        TextModel::Cursor     selectionStart;
        TextModel::Cursor     selectionEnd;
      };

    enum {
      cursorFlashTime=500
      };

    enum Element : uint32_t {
      E_None               = 0,
      E_Background         = 0x1,
      E_MenuBackground     = 0x2,
      E_MenuItemBackground = 0x4,
      // scrollbar
      E_ArrowUp            = 0x8,
      E_ArrowDown          = 0x10,
      E_ArrowLeft          = 0x20,
      E_ArrowRight         = 0x40,
      E_CentralButton      = 0x80,
      //
      E_All                = 0xFFFF
      };

    enum TextElement {
      TE_ButtonTitle,
      TE_CheckboxTitle,
      TE_LabelTitle,
      TE_TextEditContent,
      TE_LineEditContent,
      TE_MenuText1,
      TE_MenuText2,
      TE_Last
      };

    enum UIIntefaceCategory {
      UIIntefaceUnspecified=0,
      UIIntefacePC,
      UIIntefacePhone,
      UIIntefacePad
      };

    struct UIIntefaceIdiom {
      UIIntefaceIdiom(UIIntefaceCategory category);
      UIIntefaceCategory category;
      bool               touch;
      };
    virtual UIIntefaceIdiom idiom() const;

    // common
    virtual void draw(Painter& p, Widget*   w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;
    virtual void draw(Painter& p, Panel*    w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;
    virtual void draw(Painter& p, Dialog*   w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;
    virtual void draw(Painter& p, Button*   w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;
    virtual void draw(Painter& p, CheckBox* w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;
    virtual void draw(Painter& p, Label*    w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;
    virtual void draw(Painter& p, ComboBox* w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;

    // dialog shadow
    virtual void draw(Painter& p, Dialog* w, UiOverlay* o, Element e, const WidgetState& st, const Rect& r, const Extra& extra, const Rect& overlay) const;

    // complex
    virtual void draw(Painter& p, AbstractTextInput* w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;
    virtual void draw(Painter& p, ScrollBar*         w, Element e, const WidgetState& st, const Rect& r, const Extra& extra) const;

    // text
    virtual void draw(Painter& p, const TextModel& text, TextElement e, const WidgetState& st, const Rect& r, const Extra& extra) const;

    // size hint
    virtual Size sizeHint(Widget*   w, Element e, const TextModel* text, const Extra& extra) const;
    virtual Size sizeHint(Panel*    w, Element e, const TextModel* text, const Extra& extra) const;
    virtual Size sizeHint(Button*   w, Element e, const TextModel* text, const Extra& extra) const;
    virtual Size sizeHint(Label*    w, Element e, const TextModel* text, const Extra& extra) const;

    // metric
    virtual const UiMetrics& metrics() const;
    virtual const UiColors&  colors() const;
    virtual Element          visibleElements() const;

  protected:
    virtual void polish  (Widget& w) const;
    virtual void unpolish(Widget& w) const;

    static const Tempest::Sprite& iconSprite(const Icon& icon,const WidgetState &st, const Rect &r);

  private:
    mutable uint32_t refCnt   = 0;
    mutable uint32_t polished = 0;

    static const Tempest::Margin emptyMargin;
    static const Tempest::Icon   emptyIcon;
    static const Tempest::Font   emptyFont;
    static const Tempest::Color  emptyColor;

    void drawCursor(Painter &p, const WidgetState &st, int x, int h, bool animState) const;

    void implAddRef() const { refCnt++; }
    void implDecRef() const ;

  friend class Widget;
  friend class Application;
  };

}
