#pragma once

#include <Tempest/Button>

namespace Tempest {

class CheckBox : public Button {
  public:
    CheckBox();
    ~CheckBox() override;

  protected:
    void paintEvent(Tempest::PaintEvent&  e) override;
  };

}

