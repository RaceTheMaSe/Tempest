#pragma once

#include <Tempest/Widget>
#include <Tempest/Button>

namespace Tempest {

class ScrollBar : public Tempest::Widget {
  public:
    ScrollBar(Tempest::Orientation ori = Tempest::Vertical);

    void                 setOrientation(Tempest::Orientation ori);
    Tempest::Orientation orientation() const;
    Tempest::Signal<void(Tempest::Orientation)> onOrientationChanged;

    void    setRange( int min, int max );
    int64_t range() const;

    int     minValue() const;
    int     maxValue() const;

    void    setValue( int v );
    int     value()     const { return mValue; }

    int     smallStep() const { return mSmallStep; }
    int     largeStep() const { return mLargeStep; }

    void    setSmallStep(int step);
    void    setLargeStep(int step);

    void    setCentralButtonSize( int sz );
    int     centralAreaSize() const;

    void    hideArrowButtons();
    void    showArrawButtons();

    Tempest::Signal<void(int)> onValueChanged;

  protected:
    void    paintEvent     (Tempest::PaintEvent& e) override;
    void    resizeEvent    (Tempest::SizeEvent&  e) override;

    void    mouseDownEvent (Tempest::MouseEvent& e) override;
    void    mouseDragEvent (Tempest::MouseEvent& e) override;
    void    mouseUpEvent   (Tempest::MouseEvent& e) override;
    void    mouseWheelEvent(Tempest::MouseEvent& e) override;

    void    polishEvent    (Tempest::PolishEvent&) override;

  private:
    enum Elements : uint8_t {
      Elt_None  = 0,
      Elt_Inc   = 1,
      Elt_Dec   = 1<<1,
      Elt_Cen   = 1<<2,
      Elt_Space = 1<<3,
      Elt_IncL  = 1<<4,
      Elt_DecL  = 1<<5,
      };

    void     processPress();
    void     inc();
    void     dec();

    void     incL();
    void     decL();
    Rect     elementRect(Elements e) const;
    Elements tracePoint(const Point& p) const;
    void     implSetOrientation( Tempest::Orientation ori );
    auto     stateOf(WidgetState& out, Elements e) const -> WidgetState&;
    Elements elements() const;


    Timer                timer;
    int                  rmin=0, rmax=100;
    int                  mSmallStep=10, mLargeStep=20;
    int                  mValue=rmin;
    int                  cenBtnSize = 40;

    Tempest::Orientation orient  = Tempest::Vertical;
    Elements             eltMask = Elements(Elt_Inc|Elt_Dec|Elt_Cen);
    Elements             pressed = Elt_None;
    Tempest::Point       mpos;
  };
}

