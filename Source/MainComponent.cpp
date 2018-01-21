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

#include "MainComponent.h"

//==============================================================================
struct MidiDeviceListEntry : ReferenceCountedObject
{
    MidiDeviceListEntry (const String& deviceName) : name (deviceName) {}

    String name;
    ScopedPointer<MidiInput> inDevice;
    ScopedPointer<MidiOutput> outDevice;

    typedef ReferenceCountedObjectPtr<MidiDeviceListEntry> Ptr;
};

//==============================================================================
struct MidiCallbackMessage : public Message
{
    MidiCallbackMessage (const MidiMessage& msg) : message (msg) {}
    MidiMessage message;
};

//==============================================================================
class MidiDeviceListBox : public ListBox,
private ListBoxModel
{
public:
    //==============================================================================
    MidiDeviceListBox (const String& name,
                       MainContentComponent& contentComponent,
                       bool isInputDeviceList)
    : ListBox (name, this),
      parent (contentComponent),
      isInput (isInputDeviceList)
    {
        setOutlineThickness (1);
        setMultipleSelectionEnabled (true);
        setClickingTogglesRowSelection (true);
    }

    //==============================================================================
    int getNumRows() override
    {
        return isInput ? parent.getNumMidiInputs()
                       : parent.getNumMidiOutputs();
    }

    //==============================================================================
    void paintListBoxItem (int rowNumber, Graphics &g,
                           int width, int height, bool rowIsSelected) override
    {
        const auto textColour = getLookAndFeel().findColour (ListBox::textColourId);

        if (rowIsSelected)
            g.fillAll (textColour.interpolatedWith (getLookAndFeel().findColour (ListBox::backgroundColourId), 0.5));


        g.setColour (textColour);
        g.setFont (height * 0.7f);

        if (isInput)
        {
            if (rowNumber < parent.getNumMidiInputs())
                g.drawText (parent.getMidiDevice (rowNumber, true)->name,
                            5, 0, width, height,
                            Justification::centredLeft, true);
        }
        else
        {
            if (rowNumber < parent.getNumMidiOutputs())
                g.drawText (parent.getMidiDevice (rowNumber, false)->name,
                            5, 0, width, height,
                            Justification::centredLeft, true);
        }
    }

    //==============================================================================
    void selectedRowsChanged (int) override
    {
        SparseSet<int> newSelectedItems = getSelectedRows();
        if (newSelectedItems != lastSelectedItems)
        {
            for (int i = 0; i < lastSelectedItems.size(); ++i)
            {
                if (! newSelectedItems.contains (lastSelectedItems[i]))
                    parent.closeDevice (isInput, lastSelectedItems[i]);
            }

            for (int i = 0; i < newSelectedItems.size(); ++i)
            {
                if (! lastSelectedItems.contains (newSelectedItems[i]))
                    parent.openDevice (isInput, newSelectedItems[i]);
            }

            lastSelectedItems = newSelectedItems;
        }
    }

    //==============================================================================
    void syncSelectedItemsWithDeviceList (const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices)
    {
        SparseSet<int> selectedRows;
        for (int i = 0; i < midiDevices.size(); ++i)
            if (midiDevices[i]->inDevice != nullptr || midiDevices[i]->outDevice != nullptr)
                selectedRows.addRange (Range<int> (i, i+1));

        lastSelectedItems = selectedRows;
        updateContent();
        setSelectedRows (selectedRows, dontSendNotification);
    }

private:
    //==============================================================================
    MainContentComponent& parent;
    bool isInput;
    SparseSet<int> lastSelectedItems;
};

void setupKnob(Slider *slider, Slider::Listener *sliderListen,
	           Label *label,
	           const char *text, KnobLookAndFeel *lookfeel)
{
	slider->setSliderStyle(Slider::Rotary);
	slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	slider->setRange(0, 127, 1);
	slider->addListener(sliderListen);
	slider->setLookAndFeel(lookfeel);
	slider->setValue(63.0f);
	
	label->attachToComponent(slider, false);
	label->setSize(slider->getWidth() * 2, slider->getHeight() * 2);
	label->setBorderSize(BorderSize<int>(5));
	label->setText(text, dontSendNotification);
	label->setJustificationType(Justification::centred);
	label->setFont(Font(24.0, Font::bold));
	label->setEditable(true);
	
}

void setupButton(ImageButton *button, Label *label, const char *text, Image &pressed, Image &unpressed)
{
	button->setImages(false, true, true, unpressed, 1.0f, Colours::transparentBlack, Image(), 1.0f, Colour(), pressed, 1.0f, Colours::transparentBlack, 0.5f);

	label->setBorderSize(BorderSize<int>(5));
	label->setText(text, dontSendNotification);
	label->attachToComponent(button, false);
	label->setJustificationType(Justification::centred);
	label->setFont(Font(24.0, Font::bold));
	label->setEditable(true);
}


PedalAreaComponent::PedalAreaComponent()
{
}

PedalAreaComponent::~PedalAreaComponent()
{
}

void PedalAreaComponent::paint(Graphics &g)
{
	const float margin = 5.0f;

	const float thickness = 5.0f;
	const float cornerSize = 5.0f;
	g.setColour(Colours::black);
	g.fillRoundedRectangle(margin, margin, getWidth() - 2 * margin, getHeight() - 2 * margin, cornerSize);
	g.setColour(BAColour);
	g.drawRoundedRectangle(margin, margin, getWidth() - 2 * margin, getHeight() - 2 * margin, cornerSize, thickness);

	// Draw the logo
	//Image logo = ImageCache::getFromFile(File("logo_transparent.png"));
	Image logo = ImageCache::getFromMemory(BinaryData::logo_transparent_png, BinaryData::logo_transparent_pngSize);
	if (logo.isValid()) {
		float ratio = (float)logo.getWidth() / (float)logo.getHeight();
		float outputHeight = (float)getHeight() / 3.5f;
		float outputWidth = outputHeight * ratio;
		g.drawImage(logo, getWidth() / 2 - (int)(outputWidth / 2), getHeight() - (int)outputHeight - 3*(int)margin, (int)outputWidth, (int)outputHeight,
			0, 0, logo.getWidth(), logo.getHeight());
	}

}

void PedalAreaComponent::resized()
{

}

//==============================================================================
MainContentComponent::MainContentComponent ()
    : midiInputLabel ("Midi Input Label", "MIDI Input:"),
      midiOutputLabel ("Midi Output Label", "MIDI Output:"),
      incomingMidiLabel ("Incoming Midi Label", "Received MIDI messages:"),
      outgoingMidiLabel ("Outgoing Midi Label", "Play the keyboard to send MIDI messages..."),
      midiKeyboard (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
      midiMonitor ("MIDI Monitor"),
      pairButton ("MIDI Bluetooth devices..."),
	  buttonA ("A"),
	  buttonB("B"),
	  knob1 ("1"),
	  knob2("2"),
	  knob3("3"),
	  knob4("4"),
      midiInputSelector (new MidiDeviceListBox ("Midi Input Selector", *this, true)),
      midiOutputSelector (new MidiDeviceListBox ("Midi Input Selector", *this, false))
{
    setSize (APP_WIDTH, APP_HEIGHT);

    addLabelAndSetStyle (midiInputLabel);
    addLabelAndSetStyle (midiOutputLabel);
    addLabelAndSetStyle (incomingMidiLabel);
    addLabelAndSetStyle (outgoingMidiLabel);

    midiKeyboard.setName ("MIDI Keyboard");
    addAndMakeVisible (midiKeyboard);

    midiMonitor.setMultiLine (true);
    midiMonitor.setReturnKeyStartsNewLine (false);
    midiMonitor.setReadOnly (true);
    midiMonitor.setScrollbarsShown (true);
    midiMonitor.setCaretVisible (false);
    midiMonitor.setPopupMenuEnabled (false);
    midiMonitor.setText (String());
    addAndMakeVisible (midiMonitor);

    if (! BluetoothMidiDevicePairingDialogue::isAvailable())
        pairButton.setEnabled (false);

    addAndMakeVisible (pairButton);
    pairButton.addListener (this);

	// Button A/B Setup
	addAndMakeVisible(pedalArea);
	buttonLookAndFeel.setColour(TextButton::buttonOnColourId, Colours::green);

	//pressedButtonImg = ImageCache::getFromFile(File("led-circle-red-md.png"));
	pressedButtonImg = ImageCache::getFromMemory(BinaryData::ledcircleredmd_png, BinaryData::ledcircleredmd_pngSize);
	//unpressedButtonImg = ImageCache::getFromFile(File("led-circle-grey-md.png"));
	unpressedButtonImg = ImageCache::getFromMemory(BinaryData::ledcirclegreymd_png, BinaryData::ledcirclegreymd_pngSize);

	addAndMakeVisible(buttonA);	
	setupButton(&buttonA, &buttonALabel, "A", pressedButtonImg, unpressedButtonImg);
	buttonA.addListener (this);
	buttonA.setClickingTogglesState(true);
	addAndMakeVisible(buttonALabel);
	//buttonA.setLookAndFeel(&buttonLookAndFeel);

	addAndMakeVisible(buttonB);
	setupButton(&buttonB, &buttonBLabel, "B", pressedButtonImg, unpressedButtonImg);
	buttonB.addListener(this);
	buttonB.setClickingTogglesState(true);
	addAndMakeVisible(buttonBLabel);
	//buttonB.setLookAndFeel(&buttonLookAndFeel);
	
	// Knob 1/2/3/4 Setup
	setupKnob(&knob1, this, &knob1Label, "KNOB 1", &knobLookAndFeel);
	addAndMakeVisible(knob1);
	addAndMakeVisible(knob1Label);

	setupKnob(&knob2, this, &knob2Label, "KNOB 2", &knobLookAndFeel);
	addAndMakeVisible(knob2);
	addAndMakeVisible(knob2Label);

	setupKnob(&knob3, this, &knob3Label, "KNOB 3", &knobLookAndFeel);
	addAndMakeVisible(knob3);
	addAndMakeVisible(knob3Label);

	setupKnob(&knob4, this, &knob4Label, "KNOB 4", &knobLookAndFeel);
	addAndMakeVisible(knob4);
	addAndMakeVisible(knob4Label);

    keyboardState.addListener (this);
    addAndMakeVisible (midiInputSelector);
    addAndMakeVisible (midiOutputSelector);

    startTimer (500);
}

//==============================================================================
void MainContentComponent::addLabelAndSetStyle (Label& label)
{
    label.setFont (Font (15.00f, Font::plain));
    label.setJustificationType (Justification::centredLeft);
    label.setEditable (false, false, false);
    label.setColour (TextEditor::textColourId, Colours::black);
    label.setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label);
}

//==============================================================================
MainContentComponent::~MainContentComponent()
{
    stopTimer();
    midiInputs.clear();
    midiOutputs.clear();
    keyboardState.removeListener (this);

    midiInputSelector = nullptr;
    midiOutputSelector = nullptr;
    midiOutputSelector = nullptr;
}

//==============================================================================
void MainContentComponent::paint (Graphics&)
{
}

//==============================================================================
// LOCATIONS
void MainContentComponent::resized()
{
    int margin = 20;

	const int textRowHeight = 24;

	int nextRowStart = margin;

    midiInputLabel.setBounds (margin, nextRowStart,
		(getWidth() / 2) - (2 * margin), textRowHeight);

    midiOutputLabel.setBounds ((getWidth() / 2) + margin, nextRowStart,
		(getWidth() / 2) - (2 * margin), 24); nextRowStart += textRowHeight + margin;

	const int deviceListHeight = 4 * textRowHeight;
    midiInputSelector->setBounds (margin, nextRowStart,
        (getWidth() / 2) - (2 * margin),
		deviceListHeight);

    midiOutputSelector->setBounds ((getWidth() / 2) + margin, nextRowStart,
        (getWidth() / 2) - (2 * margin),
		deviceListHeight); nextRowStart += deviceListHeight + margin;

    pairButton.setBounds (margin, nextRowStart,
                          getWidth() - (2 * margin), textRowHeight);  nextRowStart += textRowHeight + margin;

	// START CUSTOM CONTROLS
	int pedalAreaStart = nextRowStart;
	nextRowStart += 3 * margin;
	
	// KNOBS
	int prevMargin = margin;
	margin = 50;
	const int KNOB_WIDTH = 75;
	const int KNOB_HEIGHT = 75;
	int knobOffset = (getWidth() - (2 * margin)) / (NUM_KNOBS - 1);
	int nextColStart = margin;
	knob1.setBounds(margin, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextColStart += knobOffset;
	knob2.setBounds(nextColStart-KNOB_WIDTH/2, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextColStart += knobOffset;
	knob3.setBounds(nextColStart-KNOB_WIDTH/2, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextColStart += knobOffset;
	knob4.setBounds(getWidth()-margin-KNOB_WIDTH, nextRowStart, KNOB_WIDTH, KNOB_HEIGHT); nextRowStart += KNOB_HEIGHT/2 + margin;
	margin = prevMargin;

	// BUTTONS
	const int CC_BUTTON_WIDTH = 100;
	const int CC_BUTTON_HEIGHT = 50;
	int buttonOffset = (getWidth() - (2 * margin)) / (NUM_BUTTONS+2);
	nextColStart = margin;
	buttonA.setBounds(margin+buttonOffset-CC_BUTTON_WIDTH/2, nextRowStart, CC_BUTTON_WIDTH, CC_BUTTON_HEIGHT);  nextColStart += 2*buttonOffset;
	buttonB.setBounds(getWidth()-margin-buttonOffset-CC_BUTTON_WIDTH/2, nextRowStart, CC_BUTTON_WIDTH, CC_BUTTON_HEIGHT);  nextRowStart += CC_BUTTON_HEIGHT + margin;

	nextRowStart += 24;

	int pedalAreaStop = nextRowStart;

	pedalArea.setBounds(margin, pedalAreaStart, getWidth() -2*margin, pedalAreaStop - pedalAreaStart);
	// END CUSTOM CONSTROLS
	outgoingMidiLabel.setBounds(margin, nextRowStart, getWidth() - (2 * margin), textRowHeight); nextRowStart += textRowHeight + margin;

	const int midiKeyboardHeight = 64;
	midiKeyboard.setBounds(0, nextRowStart, getWidth(), midiKeyboardHeight); nextRowStart += midiKeyboardHeight + margin;

    incomingMidiLabel.setBounds (margin, nextRowStart,
                                 getWidth() - (2*margin), textRowHeight); nextRowStart += textRowHeight + margin;

    midiMonitor.setBounds (margin/2, nextRowStart,
                           getWidth() - margin, getHeight() - nextRowStart - margin);
}

//==============================================================================
void MainContentComponent::buttonClicked(Button* buttonThatWasClicked)
{
	bool sendMidi = false;
	int val;
	MidiMessage *m = nullptr;
	if (buttonThatWasClicked == &pairButton)
		RuntimePermissions::request(
			RuntimePermissions::bluetoothMidi,
			[](bool wasGranted) { if (wasGranted) BluetoothMidiDevicePairingDialogue::open(); });
	if (buttonThatWasClicked == &buttonA) {
		// Send the midi CC		
		if (!buttonA.getToggleState()) { val = CC_OFF; }
		else { val = CC_ON; }
		m = new MidiMessage(MidiMessage::controllerEvent(0, buttonACCId, val));
		sendMidi = true;
	} else if (buttonThatWasClicked == &buttonB) {
		// Send the midi CC		
		if (!buttonB.getToggleState()) { val = CC_OFF; }
		else { val = CC_ON; }
		m = new MidiMessage(MidiMessage::controllerEvent(0, buttonBCCId, val));
		sendMidi = true;
	}

	if (sendMidi && m) {
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
		delete m;
	}
}

//==============================================================================
void MainContentComponent::sliderValueChanged(Slider* slider)
{
	bool sendMidi = false;
	MidiMessage *m = nullptr;
	if (slider == &knob1) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob1CCId, (int)knob1.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
    } else if (slider == &knob2) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob2CCId, (int)knob2.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
	} else if (slider == &knob3) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob3CCId, (int)knob3.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
	} else if (slider == &knob4) {
		m = new MidiMessage(MidiMessage::controllerEvent(0, knob4CCId, (int)knob4.getValue()));
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
	}

	if (sendMidi && m) {
		m->setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
		sendToOutputs(*m);
		delete m;
	}
}

//==============================================================================
bool MainContentComponent::hasDeviceListChanged (const StringArray& deviceNames, bool isInputDevice)
{
    ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                            : midiOutputs;

    if (deviceNames.size() != midiDevices.size())
        return true;

    for (int i = 0; i < deviceNames.size(); ++i)
        if (deviceNames[i] != midiDevices[i]->name)
            return true;

    return false;
}

MidiDeviceListEntry::Ptr MainContentComponent::findDeviceWithName (const String& name, bool isInputDevice) const
{
    const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                                  : midiOutputs;

    for (int i = 0; i < midiDevices.size(); ++i)
        if (midiDevices[i]->name == name)
            return midiDevices[i];

    return nullptr;
}

void MainContentComponent::closeUnpluggedDevices (StringArray& currentlyPluggedInDevices, bool isInputDevice)
{
    ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                            : midiOutputs;

    for (int i = midiDevices.size(); --i >= 0;)
    {
        MidiDeviceListEntry& d = *midiDevices[i];

        if (! currentlyPluggedInDevices.contains (d.name))
        {
            if (isInputDevice ? d.inDevice != nullptr
                              : d.outDevice != nullptr)
                closeDevice (isInputDevice, i);

            midiDevices.remove (i);
        }
    }
}

void MainContentComponent::updateDeviceList (bool isInputDeviceList)
{
    StringArray newDeviceNames = isInputDeviceList ? MidiInput::getDevices()
                                                   : MidiOutput::getDevices();

    if (hasDeviceListChanged (newDeviceNames, isInputDeviceList))
    {

        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices
            = isInputDeviceList ? midiInputs : midiOutputs;

        closeUnpluggedDevices (newDeviceNames, isInputDeviceList);

        ReferenceCountedArray<MidiDeviceListEntry> newDeviceList;

        // add all currently plugged-in devices to the device list
        for (int i = 0; i < newDeviceNames.size(); ++i)
        {
            MidiDeviceListEntry::Ptr entry = findDeviceWithName (newDeviceNames[i], isInputDeviceList);

            if (entry == nullptr)
                entry = new MidiDeviceListEntry (newDeviceNames[i]);

            newDeviceList.add (entry);
        }

        // actually update the device list
        midiDevices = newDeviceList;

        // update the selection status of the combo-box
        if (MidiDeviceListBox* midiSelector = isInputDeviceList ? midiInputSelector : midiOutputSelector)
            midiSelector->syncSelectedItemsWithDeviceList (midiDevices);
    }
}

//==============================================================================
void MainContentComponent::timerCallback ()
{
    updateDeviceList (true);
    updateDeviceList (false);
}

//==============================================================================
void MainContentComponent::handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
    m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
    sendToOutputs (m);
}

//==============================================================================
void MainContentComponent::handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber, velocity));
    m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
    sendToOutputs (m);
}

//==============================================================================
void MainContentComponent::sendToOutputs(const MidiMessage& msg)
{
    for (int i = 0; i < midiOutputs.size(); ++i)
        if (midiOutputs[i]->outDevice != nullptr)
            midiOutputs[i]->outDevice->sendMessageNow (msg);
}

//==============================================================================
void MainContentComponent::handleIncomingMidiMessage (MidiInput* /*source*/, const MidiMessage &message)
{
    // This is called on the MIDI thread
	postMessage(new MidiCallbackMessage(message));

}

//==============================================================================
void MainContentComponent::handleMessage (const Message& msg)
{
    // This is called on the message loop

    const MidiMessage& mm = dynamic_cast<const MidiCallbackMessage&> (msg).message;
    String midiString;
	
	midiString << mm.getDescription() << "\n";
    midiMonitor.insertTextAtCaret (midiString);
}

//==============================================================================
void MainContentComponent::openDevice (bool isInput, int index)
{
    if (isInput)
    {
        jassert (midiInputs[index]->inDevice == nullptr);
        midiInputs[index]->inDevice = MidiInput::openDevice (index, this);

        if (midiInputs[index]->inDevice == nullptr)
        {
            DBG ("MainContentComponent::openDevice: open input device for index = " << index << " failed!" );
            return;
        }

        midiInputs[index]->inDevice->start();
    }
    else
    {
        jassert (midiOutputs[index]->outDevice == nullptr);
        midiOutputs[index]->outDevice = MidiOutput::openDevice (index);

        if (midiOutputs[index]->outDevice == nullptr)
            DBG ("MainContentComponent::openDevice: open output device for index = " << index << " failed!" );
    }
}

//==============================================================================
void MainContentComponent::closeDevice (bool isInput, int index)
{
    if (isInput)
    {
        jassert (midiInputs[index]->inDevice != nullptr);
        midiInputs[index]->inDevice->stop();
        midiInputs[index]->inDevice = nullptr;
    }
    else
    {
        jassert (midiOutputs[index]->outDevice != nullptr);
        midiOutputs[index]->outDevice = nullptr;
    }
}

//==============================================================================
int MainContentComponent::getNumMidiInputs() const noexcept
{
    return midiInputs.size();
}

//==============================================================================
int MainContentComponent::getNumMidiOutputs() const noexcept
{
    return midiOutputs.size();
}

//==============================================================================
ReferenceCountedObjectPtr<MidiDeviceListEntry>
MainContentComponent::getMidiDevice (int index, bool isInput) const noexcept
{
    return isInput ? midiInputs[index] : midiOutputs[index];
}
