/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

//==============================================================================

class MidiDeviceListBox;
struct MidiDeviceListEntry;

#ifndef GLOBAL_STUFF
#define GLOBAL_STUFF
const Colour BAColour(56, 148, 149);
#endif

class KnobLookAndFeel : public LookAndFeel_V4
{
public:
	void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
		const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override
	{
		(void)slider;
		const float radius = jmin(width / 2, height / 2) - 4.0f;
		const float centreX = x + width * 0.5f;
		const float centreY = y + height * 0.5f;
		const float rx = centreX - radius;
		const float ry = centreY - radius;
		const float rw = radius * 2.0f;
		const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
		const float outlineWidth = 5.0f;
		const float pointerWidth = 5.0f;

		// fill
		g.setColour(Colours::lightgrey);
		g.fillEllipse(rx, ry, rw, rw);

		// outline
		g.setColour(BAColour);
		g.drawEllipse(rx, ry, rw, rw, outlineWidth);

		Path p;
		const float pointerLength = radius * 0.66f;
		const float pointerThickness = pointerWidth;
		p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
		p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));

		// pointer
		g.setColour(Colours::black);
		g.fillPath(p);
	}
};

//==============================================================================
class PedalAreaComponent : public Component
{
public:
//==============================================================================
	PedalAreaComponent();
	~PedalAreaComponent();

	void paint(Graphics& g) override;
	void resized() override;
private:
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PedalAreaComponent)
};

//==============================================================================
class ParamLabel : public Label
{
public:
	ParamLabel(const String &componentName = String(), const String &labelText = String(), String labelName = String())
		: Label(componentName, labelText), m_labelName(labelName){}

	void setLabelName(String labelName) { m_labelName = labelName;  }
	String &getLabelName(void) { return m_labelName;  }
private:
	String m_labelName;

};

//==============================================================================
class MainContentComponent  : public Component,
                              private Timer,
                              private MidiKeyboardStateListener,
                              private MidiInputCallback,
                              private MessageListener,
                              //private Button::Listener,
							  ImageButton::Listener,
	                          private Slider::Listener,
	                          private ParamLabel::Listener,
	                          private TextEditor::Listener
{
public:
    //==============================================================================
    MainContentComponent ();
    ~MainContentComponent();

    //==============================================================================
    void timerCallback () override;
    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleMessage (const Message& msg) override;

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
	void sliderValueChanged(Slider* slider) override;
	void labelTextChanged(Label *label);
	void textEditorTextChanged(TextEditor &editor);

    void openDevice (bool isInput, int index);
    void closeDevice (bool isInput, int index);

    int getNumMidiInputs() const noexcept;
    int getNumMidiOutputs() const noexcept;

    ReferenceCountedObjectPtr<MidiDeviceListEntry> getMidiDevice (int index, bool isInputDevice) const noexcept;
private:
    //==============================================================================
    void handleIncomingMidiMessage (MidiInput *source, const MidiMessage &message) override;
    void sendToOutputs(const MidiMessage& msg);

    //==============================================================================
    bool hasDeviceListChanged (const Array<MidiDeviceInfo>& availableDevices, bool isInputDevice);
    ReferenceCountedObjectPtr<MidiDeviceListEntry> findDevice (MidiDeviceInfo device, bool isInputDevice) const;
    void closeUnpluggedDevices (const Array<MidiDeviceInfo>& currentlyPluggedInDevices, bool isInputDevice);
    void updateDeviceList (bool isInputDeviceList);

    //==============================================================================
    void addLabelAndSetStyle (Label& label);

    //==============================================================================
	std::unique_ptr<XmlElement> paramTree = nullptr;
	//ValueTree paramNamesTree = ValueTree(String("params"));
	//ValueTree paramNamesTree = ValueTree(String("params"), { {"p"},{"1"} });

	LookAndFeel_V4 buttonLookAndFeel; // [1]
	KnobLookAndFeel knobLookAndFeel;

    Label midiInputLabel;
    Label midiOutputLabel;
    Label incomingMidiLabel;
    Label outgoingMidiLabel;
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent midiKeyboard;
    TextEditor midiMonitor;
    TextButton pairButton;

	const int APP_WIDTH  = 740;
	const int APP_HEIGHT = 800;
	// Custom Controls
	PedalAreaComponent pedalArea;
	TextButton loadButton;
	TextButton saveButton;

	// Midi Channel Stuff
	Label midiChannelLabel;
	TextEditor midiChannelText;
	int midiChannel = 1;

	const int NUM_KNOBS = 4;
	ParamLabel  effectLabel;
	Slider knob1;
	ParamLabel  knob1Label;
	Slider knob2;
	ParamLabel  knob2Label;
	Slider knob3;
	ParamLabel  knob3Label;
	Slider knob4;
	ParamLabel  knob4Label;

	const int NUM_BUTTONS = 2;
	Image pressedButtonImg;
	Image unpressedButtonImg;
	ImageButton buttonA;
	ParamLabel buttonALabel;
	ImageButton buttonB;
	ParamLabel buttonBLabel;
	//TextButton buttonA;
	//TextButton buttonB;

	const int CC_ON = 0xff;
	const int CC_OFF = 0;
	const int buttonACCId = 16;
	const int buttonBCCId = buttonACCId + 1;
	const int knob1CCId = 20;
	const int knob2CCId = knob1CCId + 1;
	const int knob3CCId = knob2CCId + 1;
	const int knob4CCId = knob3CCId + 1;

    ScopedPointer<MidiDeviceListBox> midiInputSelector;
    ScopedPointer<MidiDeviceListBox> midiOutputSelector;

    ReferenceCountedArray<MidiDeviceListEntry> midiInputs;
    ReferenceCountedArray<MidiDeviceListEntry> midiOutputs;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

