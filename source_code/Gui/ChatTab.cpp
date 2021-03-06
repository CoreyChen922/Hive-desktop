#include "ChatTab.h"
#include <QApplication>
#include <QMenu>
#include <QDebug>
#include <QScrollBar>

ChatTab::ChatTab(QWidget *parent) : QWidget(parent)
{

  comb_scrollarea = new QScrollArea(this);
  comb_scrollarea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  comb_scrollarea->setFrameShape(QFrame::NoFrame);
  comb_scrollarea->setWidgetResizable(true);
  comb_scroll_widget = new ChatTab_comb_scroll_widget();
  comb_scrollarea->setWidget(comb_scroll_widget);

  main_layout = new QVBoxLayout(this);
  main_layout->addWidget(comb_scrollarea);
  main_layout->setContentsMargins(5,5,5,5);

  QPalette palette;
  palette.setColor(QPalette::Window, GlobalData::color_tab);

  this->setAutoFillBackground(true);
  this->setPalette(palette);
  this->setFixedWidth(250);
}



//////////////////////////////////////////////////////////////////////////////////////////////////

ChatTab_comb_scroll_widget::ChatTab_comb_scroll_widget(QWidget *parent) : QWidget(parent)
{
  QPalette palette;
  palette.setColor(QPalette::Window,QColor(0,0,0,0));// GlobalData::g_tabColor);
  this->setPalette(palette);
  this->setAutoFillBackground(true);

  ///main layout
  main_layout = new QVBoxLayout(this);
  main_layout->setAlignment(Qt::AlignTop);
  main_layout->setContentsMargins(0,0,0,0);
  main_layout->setSpacing(0);
}

void ChatTab_comb_scroll_widget::addComb(UsrProfileStruct *usrProfileStruct)
{
  Log::gui(Log::Normal, "GuiChatTab_comb_scroll_widget::addComb()", "Added a comb.");
  CombWidget *comb_widget = new CombWidget(usrProfileStruct, this);
  comb_widget_hash.insert(usrProfileStruct->key, comb_widget);
  main_layout->addWidget(comb_widget);

  connect(comb_widget, &CombWidget::clicked,
          [this](const QString &usrKey){
            emit combWidgetClicked(usrKey);
          });
}

void ChatTab_comb_scroll_widget::refreshBadgeNumber(const QString &usrKey, const int &num)
{
  CombWidget *comb_widget = comb_widget_hash.value(usrKey);
  qDebug()<<"badge:" <<num;
  comb_widget->setBadgeNumber(num);
  if(num != 0)
    {
      main_layout->takeAt(main_layout->indexOf(comb_widget));
      main_layout->insertWidget(0, comb_widget);
    }
}

bool ChatTab_comb_scroll_widget::contains(const QString &usrKey)
{
  return comb_widget_hash.contains(usrKey);
}


void ChatTab_comb_scroll_widget::refreshComb(UsrProfileStruct *usrProfileStruct)
{
  CombWidget *comb_widget = comb_widget_hash.value(usrProfileStruct->key);
  if(comb_widget != nullptr)
    {
      comb_widget->setProfile(usrProfileStruct);
    }
  else
    {
      qDebug()<<"Something Impossible happened!!!";
    }
}

void ChatTab_comb_scroll_widget::onCombWidgetClicked(const QString &usrKey)
{
  emit combWidgetClicked(usrKey);
}
