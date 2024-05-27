/*****************************************************
 *	C++ code generated with Nana Creator (0.32.0)
 *	GitHub repo: https://github.com/besh81/nana-creator
 *
 * PLEASE EDIT ONLY INSIDE THE TAGS:
 *		//<*tag
 *			user code
 *		//*>
*****************************************************/

#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <nana/gui.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/picture.hpp>

//<*includes

#include "global.h"
#include <opencv2/videoio.hpp>

//debug
#include <iostream>

//*>


class FormMain
	: public nana::form
{
public:
	FormMain(nana::window wd, const ::nana::size& sz = {640, 550}, const nana::appearance& apr = {true, true, false, false, false, false, false})
		: nana::form(wd, sz, apr)
	{
		init_();

		//<*ctor
		events().unload(std::bind([]() { global::stop = true; }));												// (Upper right X clicked)
		MenuMain.at(0).answerer(0, std::bind(&FormMain::ClickExit,this,std::placeholders::_1));	// File -> Exit clicked

		// get list of available cameras
		cv::VideoCapture vc;
		for (int i = 0; i < 20; i++)
		{
			if (vc.open(i))
			{
				ComboCamera.push_back(vc.getBackendName() + " Camera " + std::to_string(i));
			}
		}

		//ComboCamera.events().text_changed(std::bind([this]() { std::cout << ComboCamera.option(); }));

		//*>
	}

	~FormMain()
	{
		//<*dtor

		//*>
	}

	void ClickExit(nana::menu::item_proxy& ip)		// Exit clicked on menu;
	{
		global::stop = true;
		this->close();
	}

	nana::picture *GetPictureCamera()
	{
		return &PictureCamera;
	}

	nana::button* GetButtonStartStop()
	{
		return &ButtonStartStop;
	}

	nana::combox* GetComboCamera()
	{
		return &ComboCamera;
	}

	nana::label* GetLabelStatus()
	{
		return &LabelStatus;
	}

	nana::combox* GetComboTrack()
	{
		return &ComboTrack;
	}

	nana::label* GetLabelTrackStatus()
	{
		return &LabelTrackStatus;
	}

private:
	void init_()
	{
		place_.bind(*this);
		place_.div("vert <vert weight=26 gap=2 FieldMenu><vert margin=[5,5,5,5] gap=2 arrange=[26,480] FieldTop>");
		caption("Restimulator");
		// MenuMain
		MenuMain.create(*this);
		place_["FieldMenu"] << MenuMain;
		auto* MenuMain_0_submenu_ = &MenuMain.push_back("File");
		MenuMain_0_submenu_->append("Exit");
		// PanelTop
		PanelTop.create(*this);
		PanelTop_place_.bind(PanelTop);
		PanelTop_place_.div("weight=26 gap=2 arrange=[380,100,variable] _field_");
		place_["FieldTop"] << PanelTop;
		// ComboCamera
		ComboCamera.create(PanelTop);
		PanelTop_place_["_field_"] << ComboCamera;
		ComboCamera.option(0);
		// ButtonStartStop
		ButtonStartStop.create(PanelTop);
		PanelTop_place_["_field_"] << ButtonStartStop;
		ButtonStartStop.caption("Start/Stop");
		// LabelStatus
		LabelStatus.create(PanelTop);
		PanelTop_place_["_field_"] << LabelStatus;
		LabelStatus.caption("Status");
		LabelStatus.text_align(static_cast<nana::align>(0), static_cast<nana::align_v>(1));
		// panel1
		panel1.create(*this);
		panel1_place_.bind(panel1);
		panel1_place_.div("weight=480 gap=2 arrange=[480,variable] _field_");
		place_["FieldTop"] << panel1;
		// PictureCamera
		PictureCamera.create(panel1);
		panel1_place_["_field_"] << PictureCamera;
		PictureCamera.align(static_cast<nana::align>(0), static_cast<nana::align_v>(0));
		// PanelInfo
		PanelInfo.create(panel1);
		PanelInfo_place_.bind(PanelInfo);
		PanelInfo_place_.div("vert gap=2 arrange=[40,26,variable] _field_");
		panel1_place_["_field_"] << PanelInfo;
		// LabelTrack
		LabelTrack.create(PanelInfo);
		PanelInfo_place_["_field_"] << LabelTrack;
		LabelTrack.caption("Track");
		LabelTrack.text_align(static_cast<nana::align>(0), static_cast<nana::align_v>(2));
		// ComboTrack
		ComboTrack.create(PanelInfo);
		PanelInfo_place_["_field_"] << ComboTrack;
		ComboTrack.push_back(" ");
		ComboTrack.push_back("Head");
		ComboTrack.push_back("Hips");
		ComboTrack.push_back("Left Hand");
		ComboTrack.push_back("Right Hand");
		ComboTrack.push_back("Left Foot");
		ComboTrack.push_back("Right Foot");
		ComboTrack.option(0);
		// LabelTrackStatus
		LabelTrackStatus.create(PanelInfo);
		PanelInfo_place_["_field_"] << LabelTrackStatus;
		LabelTrackStatus.caption("Tracking Status");

		place_.collocate();
		PanelTop_place_.collocate();
		panel1_place_.collocate();
		PanelInfo_place_.collocate();
	}


protected:
	nana::place place_;
	nana::menubar MenuMain;
	nana::panel<true> PanelTop;
	nana::place PanelTop_place_;
	nana::combox ComboCamera;
	nana::button ButtonStartStop;
	nana::label LabelStatus;
	nana::panel<true> panel1;
	nana::place panel1_place_;
	nana::picture PictureCamera;
	nana::panel<true> PanelInfo;
	nana::place PanelInfo_place_;
	nana::label LabelTrack;
	nana::combox ComboTrack;
	nana::label LabelTrackStatus;


	//<*declarations

	//*>
};

#endif //FORMMAIN_H

