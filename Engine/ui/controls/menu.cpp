#include "menu.h"

#include <Tempest/Layout>
#include <Tempest/Panel>
#include <Tempest/ListView>
#include <Tempest/ScrollWidget>
#include <Tempest/Painter>

using namespace Tempest;

struct Menu::Delegate : public ListDelegate {
    Delegate( Menu& owner, const std::vector<Menu::Item>& items )
      :owner(owner), items(items){}

    size_t  size() const override { return items.size(); }
    Widget* createView(size_t position) override {
      return owner.createItem(items[position]);
      }
    void    removeView(Widget* w, size_t /*position*/) override{
      delete w;
      }

    Menu&                          owner;
    const std::vector<Menu::Item>& items;
  };

Menu::Overlay::Overlay(Menu* menu, Widget* owner)
  :owner(owner), menu(menu) {
  }

Menu::Overlay::~Overlay() {
  menu->overlay = nullptr;
  }

void Menu::Overlay::mouseDownEvent(MouseEvent &e) {
  const Point pt0 = mapToRoot(e.pos());
  const Point pt1 = owner->mapToRoot(Point());
  const Point pt = pt0-pt1;
  if(!(pt.x>=0 && pt.y>=0 && pt.x<owner->w() && pt.y<owner->h()))
    e.ignore(); else
    e.accept();
  delete this;
  }

void Menu::Overlay::mouseMoveEvent(MouseEvent& e) {
  e.accept();
  }

void Menu::Overlay::resizeEvent(SizeEvent&) {
  invalidatePos();
  }

void Menu::Overlay::invalidatePos() {
  Widget* parent = owner==nullptr ? this : owner;
  for(auto& i:menu->panels) {
    if(i.widget==nullptr)
      continue;
    auto pos = parent->mapToRoot(i.pos);
    i.widget->setPosition(pos);
    //i.widget->resize(i.widget->sizeHint());
    menu->setupMenuPanel(*i.widget);
    parent = i.widget;
    }
  }


Menu::Menu(Declarator&& decl): decl(std::move(decl)){
  }

Menu::~Menu() {
  implClose();
  }

void Menu::setMinimumWidth(int w) {
  minW = w;
  }

int Menu::exec(Widget &owner) {
  return exec(owner,Tempest::Point(0,owner.h()));
  }

int Menu::exec(Widget& owner, const Point &pos, bool alignWidthToOwner) {
  if(overlay!=nullptr)
    implClose();

  running = true;
  overlay = new Overlay(this,&owner);
  overlay->setStyle(&owner.style());
  SystemApi::addOverlay(overlay);

  decl.level = 0;
  initMenuLevels(decl);

  Widget *box = createDropList(owner,alignWidthToOwner,decl.items);
  if(box==nullptr) {
    implClose();
    return -1;
    }
  panels.resize(1);
  panels[0].widget = box;
  panels[0].pos    = pos;
  overlay->addWidget( box );
  box->setFocus(true);

  while(overlay!=nullptr && running && Application::isRunning()) {
    Application::processEvents();
    }
  return 0;
  }

void Menu::close() {
  running = false;
  }

void Menu::implClose() {
  if(overlay!=nullptr){
    delete overlay;
    overlay = nullptr;
    }
  }

Widget *Menu::createDropList( Widget& owner,
                              bool alignWidthToOwner,
                              const std::vector<Item>& items ) {
  Widget* list = createItems(items);
  Size sz = list->size();

  sz.w = std::max(minW,sz.w);

  auto *box = new MenuPanel();
  box->setMargins(Margin(0));
  sz.w+=box->margins().xMargin();
  sz.h+=box->margins().yMargin();

  if(alignWidthToOwner) {
    sz.w = std::max(owner.w(),sz.w);
    }

  box->setSize(sz);
  box->addWidget(list);
  box->setLayout(Tempest::Horizontal);
  return box;
  }

Widget *Menu::createItems(const std::vector<Item>& items) {
  auto* list = new ListView(Vertical);
  list->setDelegate(new Delegate(*this,items));
  auto sz = list->centralWidget().sizeHint();
  list->resize(sz);
  return list;
  }

Widget *Menu::createItem(const Menu::Item &declr) {
  auto* b = new ItemButton(declr.items);

  b->setText(declr.text);
  b->setExtraText(declr.text2);
  b->setIcon(declr.icon);
  b->onClick.bind(&declr.activated,&Signal<void()>::operator());
  b->onClick.bind(this,&Menu::close);
  b->onMouseEnter.bind(this,&Menu::openSubMenu);

  return b;
  }

void Menu::openSubMenu(const Declarator &declr, Widget &owner) {
  if(panels.size()<=declr.level)
    panels.resize(declr.level+1);

  for(size_t i=declr.level;i<panels.size(); ++i){
    if(panels[i].widget!=nullptr) {
      delete panels[i].widget;
      panels[i].widget = nullptr;
      }
    }

  if(!overlay || overlay->owner==nullptr || declr.items.size()==0)
    return;

  Widget *box = createDropList(*overlay->owner,false,declr.items);
  if(!box)
    return;
  box->setEnabled(owner.isEnabled());
  Widget* root = declr.level>0 ? panels[declr.level-1].widget : nullptr;

  panels[declr.level].widget = box;
  if(root!=nullptr)
    panels[declr.level].pos = Point(root->w(),owner.y()); else
    panels[declr.level].pos = Point(owner.w(),0);
  overlay->addWidget(box);
  box->setFocus(true);
  overlay->invalidatePos();
  }

void Menu::setupMenuPanel(Widget& box) {
  if(box.w()>overlay->w())
    box.resize(overlay->w(), box.h());

  if( box.h() > overlay->h() )
    box.resize( box.w(), overlay->h());

  if(box.y()+box.h() > overlay->h())
    box.setPosition(box.x(), overlay->h()-box.h());
  if(box.x()+box.w() > overlay->w())
    box.setPosition(overlay->w()-box.w(), box.y());
  if(box.x()<0)
    box.setPosition(0, box.y());
  if(box.y()<0)
    box.setPosition(box.x(), 0);

  box.setFocus(true);
  }

void Menu::initMenuLevels(Menu::Declarator &declr) {
  for(Item& i:declr.items){
    i.items.level = declr.level+1;
    initMenuLevels(i.items);
    }
  }

void Menu::assign(std::string &s, const char *ch) {
  s = ch;
  }

void Menu::assign(std::string& s, const char16_t *ch) {
  s = TextCodec::toUtf8(ch);
  }

void Menu::assign(std::string &s, const std::string &ch) {
  s = ch;
  }

void Menu::assign(std::string &s, const std::u16string &ch) {
  s = TextCodec::toUtf8(ch);
  }


Menu::ItemButton::ItemButton(const Declarator &item):item(item){
  text2.setFont(Application::font());
  setSizePolicy(Preferred,Fixed);
  }

void Menu::ItemButton::mouseEnterEvent(MouseEvent& e) {
  Button::mouseEnterEvent(e);
  onMouseEnter(item,*this);
  update();
  }

void Menu::ItemButton::paintEvent(PaintEvent &e) {
  Tempest::Painter p(e);
  Style::Extra ex(*this);
  style().draw(p,this,  Style::E_MenuItemBackground,state(),Rect(0,0,w(),h()),ex);
  style().draw(p,text(),Style::TE_MenuText1,        state(),Rect(0,0,w(),h()),ex);
  style().draw(p,text2, Style::TE_MenuText2,        state(),Rect(0,0,w(),h()),ex);
  }

void Menu::ItemButton::setExtraText(const char* t) {
  text2.setText(t);
  }

void Menu::ItemButton::setExtraText(const std::string& t) {
  setExtraText(t.c_str());
  }

void Menu::ItemButton::setExtraFont(const Font& f) {
  text2.setFont(f);
  }

void Menu::MenuPanel::paintEvent(PaintEvent &e) {
  Tempest::Painter p(e);
  style().draw(p,this,Style::E_MenuBackground,state(),Rect(0,0,w(),h()),Style::Extra(*this));
  }

void Menu::MenuPanel::setSize(const Size& s) {
  setSizeHint(s);
  resize(s);
  }
