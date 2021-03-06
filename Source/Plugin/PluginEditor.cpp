/*
 ==============================================================================

 PluginEditor.cpp
 Author:  Daniel Lindenfelser

 ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

SoundboardAudioProcessorEditor::SoundboardAudioProcessorEditor(SoundboardAudioProcessor &p)
        : AudioProcessorEditor(&p), processor(p), mPauseState(true), mTimerState(true)
{
#if JUCE_WINDOWS
#if JUCE_OPENGL
    openGLContext.attachTo (*getTopLevelComponent());
#endif
#endif
    addAndMakeVisible(topBar = new Bar());
    
    addAndMakeVisible(gainSlider = new Slider());
    gainSlider->setRange(0, 100, 1);
    gainSlider->setValue(gainSlider->proportionOfLengthToValue(processor.getGain()), dontSendNotification);
    gainSlider->setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    gainSlider->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    gainSlider->setColour(Slider::ColourIds::thumbColourId, ThemeForeground1);
    gainSlider->setColour(Slider::ColourIds::trackColourId, ThemeBackground1);
    gainSlider->addListener(this);
    gainBubble = new BubbleMessageComponent();
    gainBubble->addToDesktop(0);
    gainBubble->setAllowedPlacement(BubbleMessageComponent::BubblePlacement::below);
    gainBubble->setColour(BubbleMessageComponent::ColourIds::backgroundColourId, ThemeBackground3);
    gainBubble->setColour(BubbleMessageComponent::ColourIds::outlineColourId, ThemeBackground1);
    
    addAndMakeVisible(loadDirectoryButton = new TextButton());
    loadDirectoryButton->setButtonText(FontAwesome_Folder_Open_O);
    loadDirectoryButton->setLookAndFeel(awesomeLookAndFeel);
    loadDirectoryButton->setConnectedEdges(TextButton::ConnectedOnRight);
    loadDirectoryButton->addListener(this);

    addAndMakeVisible(listButton = new TextButton());
    listButton->setLookAndFeel(awesomeLookAndFeel);
    listButton->setButtonText(FontAwesome_List);
    listButton->addListener(this);
    listButton->setConnectedEdges(TextButton::ConnectedOnRight);

    addAndMakeVisible(gridButton = new TextButton());
    gridButton->setLookAndFeel(awesomeLookAndFeel);
    gridButton->setButtonText(FontAwesome_Th);
    gridButton->addListener(this);
    gridButton->setConnectedEdges(TextButton::ConnectedOnLeft);

    addAndMakeVisible(settingsButton = new TextButton());
    settingsButton->setLookAndFeel(awesomeLookAndFeel);
    settingsButton->setButtonText(FontAwesome_Cog);
    settingsButton->addListener(this);
    settingsButton->setConnectedEdges(TextButton::ConnectedOnLeft);

    addAndMakeVisible(duckButton = new TextButton());
    duckButton->setLookAndFeel(awesomeLookAndFeel);
    duckButton->setButtonText(FontAwesome_Comment_O);
    duckButton->addListener(this);
    
    addAndMakeVisible(table = new SoundboardTableComponent(p));
    listButton->setEnabled(false);

    addAndMakeVisible(grid = new SoundboardGridComponent(p));
    grid->setVisible(false);
    grid->toBack();

    if (processor.wrapperType != AudioProcessor::wrapperType_Standalone)
    {
        addAndMakeVisible(resizableCornerComponent = new ResizableCornerComponent(this, &resizeLimits));
        resizeLimits.setSizeLimits(380, 320, 1024, 768);
    }

    refresh();

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(processor.getWindowWidth(), processor.getWindowHeight());

    startTimer(TimerIdBlink, static_cast<int>(1000 * 0.5));
    startTimer(TimerIdUpdate, 50);
    startTimer(TimerIdRefresh, static_cast<int>(1000 * 0.5));
    
    // listen to gain changes
    processor.getOscManager()->addOscParameterListener(this, "/ultraschall/soundboard/gain$");
}

SoundboardAudioProcessorEditor::~SoundboardAudioProcessorEditor()
{
    processor.getOscManager()->removeOscParameterListener(this);
    stopTimer(TimerIdBlink);
    stopTimer(TimerIdUpdate);
    stopTimer(TimerIdRefresh);
    topBar              = nullptr;
    loadDirectoryButton = nullptr;
    table               = nullptr;
    grid                = nullptr;
    resizableCornerComponent = nullptr;
    settingsButton      = nullptr;
    gridButton          = nullptr;
    gainSlider          = nullptr;
    gainBubble          = nullptr;
}

void SoundboardAudioProcessorEditor::paint(Graphics &g)
{
    g.fillAll(ThemeBackground1);
}

void SoundboardAudioProcessorEditor::resized()
{
    topBar->setBounds(0, 0, getWidth(), 32);


    listButton->setBounds(5, 5, 60, 24);
    gridButton->setBounds(65, 5, 60, 24);

    duckButton->setBounds(260, 5, 60, 24);
    
    gainSlider->setBounds(140, 5, 100, 24);

    loadDirectoryButton->setBounds(getWidth() - 125, 5, 60, 24);
    settingsButton->setBounds(getWidth() - 65, 5, 60, 24);

    table->setBounds(0, 32, getWidth(), getHeight() - 32);
    grid->setBounds(0, 32, getWidth(), getHeight() - 32);

    if (resizableCornerComponent)
    {
        resizableCornerComponent->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    }

    processor.storeWindowWidth(getWidth());
    processor.storeWindowHeight(getHeight());
}

void SoundboardAudioProcessorEditor::buttonClicked(Button *buttonThatWasClicked)
{
    if (loadDirectoryButton == buttonThatWasClicked)
    {
        FileChooser chooser("Open...");
        if (chooser.browseForDirectory())
        {
            auto directory = chooser.getResult();
            if (directory.isDirectory())
            {
                processor.openDirectory(directory);
                table->updateContent();
                grid->updateContent();
            }
        }
    }
    else if (settingsButton == buttonThatWasClicked)
    {
        DialogWindow::LaunchOptions launchOptions;
        launchOptions.componentToCentreAround = this;
        launchOptions.content.setOwned(new SoundboardSettingsComponent(processor));
        launchOptions.content->setSize(640, 480);
        launchOptions.dialogBackgroundColour = Colours::black;
        launchOptions.dialogTitle = TRANS("Soundboard Settings");
        launchOptions.escapeKeyTriggersCloseButton = true;
        launchOptions.resizable = true;
        launchOptions.useNativeTitleBar = true;
        launchOptions.runModal();
    }
    else if (gridButton == buttonThatWasClicked)
    {
        gridButton->setEnabled(false);
        listButton->setEnabled(true);

        table->setVisible(false);
        grid->setVisible(true);
        grid->toFront(true);
        if (resizableCornerComponent)
        {
            resizableCornerComponent->toFront(false);
        }
    }
    else if (listButton == buttonThatWasClicked)
    {
        listButton->setEnabled(false);
        gridButton->setEnabled(true);

        grid->setVisible(false);
        table->setVisible(true);
        table->toFront(true);
        if (resizableCornerComponent)
        {
            resizableCornerComponent->toFront(false);
        }
    }
    else if (duckButton == buttonThatWasClicked) {
        processor.toggleDucking();
        if (processor.isDucking()) {
            duckButton->setButtonText(FontAwesome_Comment);
        } else {
            duckButton->setButtonText(FontAwesome_Comment_O);
        }
    }
}

void SoundboardAudioProcessorEditor::timerCallback(int timerID)
{
    if (timerID == TimerIdUpdate)
    {
    }
    else if (timerID == TimerIdRefresh)
    {
        refresh();
    }
}

void SoundboardAudioProcessorEditor::refresh()
{
    if (!processor.isLoocked())
    {
        table->updateContent();
    }
}

void SoundboardAudioProcessorEditor::handleOscParameterMessage(OscParameter *parameter) {
    if (parameter->addressMatch("/ultraschall/soundboard/gain")) {
        gainSlider->setValue(gainSlider->proportionOfLengthToValue(parameter->getValue()), dontSendNotification);
        AttributedString text;
        String value(gainSlider->getValue());
        if (value.length() == 1) {
            text.append("  ");
        } else if (value.length() == 2) {
            text.append(" ");
        }
        text.append(value, ThemeForeground1);
        text.append(" %", ThemeForeground1);
        gainBubble->toFront(false);
        gainBubble->showAt(gainSlider, text, 500);
        if (grid->isVisible())
            grid->resized();
    }
}

void SoundboardAudioProcessorEditor::sliderValueChanged(Slider *slider)
{
    if (slider == gainSlider) {
        processor.getOscManager()->setOscParameterValue("/ultraschall/soundboard/gain", static_cast<float>(slider->valueToProportionOfLength(slider->getValue())));
    }
}
