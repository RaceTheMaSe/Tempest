#pragma once

#include <Tempest/Widget>
#include <Tempest/Sprite>

namespace Tempest {

/** \addtogroup GUI
 *  @{
 */
class Panel : public Tempest::Widget {
  public:
    Panel();

    void setDragable( bool d );
    bool isDragable();

  protected:
    void mouseDownEvent(Tempest::MouseEvent &e) override;
    void mouseDragEvent(Tempest::MouseEvent &e) override;
    void mouseMoveEvent(Tempest::MouseEvent &e) override;
    void mouseUpEvent(Tempest::MouseEvent &e) override;

    void mouseWheelEvent(Tempest::MouseEvent &e) override;
    //void gestureEvent   (Tempest::AbstractGestureEvent &e);

    void paintEvent(Tempest::PaintEvent &p) override;

  private:
    bool           mouseTracking=false;
    bool           dragable     =false;
    Tempest::Point mpos, oldPos;
  };
/** @}*/

}
