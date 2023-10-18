#include "dialog.h"

#include <Tempest/Application>
#include <Tempest/Layout>
#include <Tempest/Painter>
#include <Tempest/UiOverlay>

using namespace Tempest;

struct Dialog::LayShadow : Tempest::Layout {
  bool hasLay = false;

  void applyLayout() override {
    Widget& owg   = *owner();
    size_t  count = owg.widgetsCount();

    if(hasLay || owg.size().isEmpty())
      return;

    const bool sendShowEvent = !hasLay;
    hasLay = true;

    for(size_t i=0;i<count;++i){
      Widget& wg = owg.widget(i);

      if(sendShowEvent) {
        if(auto d = dynamic_cast<Dialog*>(&wg))
          d->showEvent();
        }

      Point pos = wg.pos();
      if(wg.x()+wg.w()>owg.w())
        pos.x = owg.w()-wg.w();
      if(wg.y()+wg.h()>owg.h())
        pos.y = owg.h()-wg.h();
      if(pos.x<=0)
        pos.x = 0;
      if(pos.y<=0)
        pos.y = 0;

      wg.setPosition(pos);
      }
    }
  };

struct Dialog::Overlay : public Tempest::UiOverlay {
  Dialog& dlg;

  Overlay(Dialog& dlg):dlg(dlg){}

  void mouseDownEvent(MouseEvent& e) override {
    if(dlg.modal)
      e.accept(); else
      e.ignore();
    if(dlg.popup) {
      CloseEvent ce;
      ce.accept();
      dlg.closeEvent(ce);
      if(ce.isAccepted()) {
        dlg.close();
        } else {
        ce.accept();
        }
      }
    }

  void mouseMoveEvent(MouseEvent& e) override {
    if(dlg.modal)
      e.accept(); else
      e.ignore();
    }

  void mouseWheelEvent(MouseEvent& e) override {
    if(dlg.modal)
      e.accept(); else
      e.ignore();
    }

  void paintEvent(PaintEvent& e) override {
    dlg.paintShadow(e);
    }

  void keyDownEvent(Tempest::KeyEvent& e) override {
    dlg.keyDownEvent(e);
    e.accept();
    }

  void keyUpEvent(Tempest::KeyEvent& e) override {
    dlg.keyUpEvent(e);
    e.accept();
    }

  void keyDownEvent(Tempest::GamepadKeyEvent& e) override {
    dlg.keyDownEvent(e);
    e.accept();
  }

  void keyUpEvent(Tempest::GamepadKeyEvent& e) override {
    dlg.keyUpEvent(e);
    e.accept();
  }

  void closeEvent(Tempest::CloseEvent& e) override {
    dlg.closeEvent(e);
    }
  };

Dialog::Dialog() {
  resize(300,200);
  }

Dialog::~Dialog() {
  close();
  }

int Dialog::exec() {
  if(owner_ov==nullptr){
    owner_ov = new Overlay(*this);

    SystemApi::addOverlay(std::move(owner_ov));
    owner_ov->setLayout(new LayShadow());
    owner_ov->addWidget(this);
    }

  setVisible(true);
  showEvent();
  while(owner_ov && Application::isRunning()) {
    Application::processEvents();
    }
  return 0;
  }

void Dialog::close() {
  if(owner_ov==nullptr)
    return;
  Tempest::UiOverlay* ov = owner_ov;
  owner_ov = nullptr;

  setVisible(false);
  CloseEvent e;
  this->closeEvent(e);

  ov->takeWidget(this);
  delete ov;
  }

bool Dialog::isOpen() const {
  return owner_ov!=nullptr;
  }

void Dialog::setModal(bool m) {
  modal = m;
  }

bool Dialog::isModal() const {
  return modal;
  }

void Dialog::setPopup(bool p) {
  popup = p;
  }

bool Dialog::isPopup() const {
  return popup;
  }

void Dialog::closeEvent(CloseEvent&) {
  if(owner_ov!=nullptr)
    close();
  }

void Dialog::keyDownEvent(KeyEvent &e) {
  if(e.key==KeyEvent::K_ESCAPE)
    close();
  }

void Dialog::keyUpEvent(KeyEvent& e) {
  e.ignore();
  }

void Dialog::keyDownEvent(GamepadKeyEvent &e) {
  if(e.key==GamepadKeyEvent::G_B || e.key==GamepadKeyEvent::G_Start) // and other cancel keys
    close();
}

void Dialog::keyUpEvent(GamepadKeyEvent& e) {
  e.ignore();
}

void Dialog::paintEvent(PaintEvent& e) {
  Tempest::Painter p(e);
  style().draw(p,this,Style::E_Background,state(),Rect(0,0,w(),h()),Style::Extra(*this));
  }

void Dialog::paintShadow(PaintEvent &e) {
  if(owner_ov==nullptr)
    return;
  Painter p(e);
  style().draw(p,this,owner_ov,Style::E_Background,state(),
               rect(),Style::Extra(*this),Rect(0,0,owner_ov->w(),owner_ov->h()));
  }

void Dialog::showEvent() {
  auto& owg = *owner();
  Point pos;
  pos.x = (owg.w()-this->w())/2;
  pos.y = (owg.h()-this->h())/2;
  setPosition(pos);
  }
