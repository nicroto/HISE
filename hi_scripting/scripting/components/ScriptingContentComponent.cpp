/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

ModulatorPeakMeter::ModulatorPeakMeter(Modulator *m) :
mod(m)
{
	addAndMakeVisible(vuMeter = new VuMeter());

	vuMeter->setType(VuMeter::MonoVertical);

	vuMeter->setColour(VuMeter::ledColour, Colour(0x88dddddd));

	startTimer(40);

	vuMeter->addMouseListener(this, true);
}

ScriptContentComponent::ScriptContentComponent(ProcessorWithScriptingContent *p_) :
processor(p_),
p(dynamic_cast<Processor*>(p_)),
editedComponent(-1)
{
    setNewContent(processor->getScriptingContent());

	setInterceptsMouseClicks(false, true);

	p->addChangeListener(this);
	p->getMainController()->addScriptListener(this, true);
}




ScriptContentComponent::~ScriptContentComponent()
{
	if (contentData.get() != nullptr)
	{
		contentData->removeChangeListener(this);

		for (int i = 0; i < contentData->getNumComponents(); i++)
		{
			contentData->getComponent(i)->removeChangeListener(this);
		}
	}

	if (p.get() != nullptr)
	{
		p->getMainController()->removeScriptListener(this);
		p->removeChangeListener(this);
	};
}



void ScriptContentComponent::refreshMacroIndexes()
{
	MacroControlBroadcaster *mcb = p->getMainController()->getMacroManager().getMacroChain();

	for(int i = 0; i < componentWrappers.size(); i++)
	{
		int macroIndex = mcb->getMacroControlIndexForProcessorParameter(p, i);

		if(macroIndex != -1)
		{
			MacroControlBroadcaster::MacroControlledParameterData * pData = mcb->getMacroControlData(macroIndex)->getParameterWithProcessorAndIndex(p, i);

			// Check if the name matches
			if(pData->getParameterName() != componentWrappers[i]->getComponent()->getName())
			{
				const String x = pData->getParameterName();

				mcb->getMacroControlData(macroIndex)->removeParameter(x);

				p->getMainController()->getMacroManager().getMacroChain()->sendChangeMessage();

				debugToConsole(p, "Index mismatch: Removed Macro Control for " + x);
			}

		}
	}
}


String ScriptContentComponent::getContentTooltip() const
{
	if (contentData != nullptr)
	{
		return contentData->tooltip;
	}
	else return String();
}

Colour ScriptContentComponent::getContentColour()
{
	if (contentData != nullptr)
	{
		return contentData->colour;
	}
	else
	{
		return Colour(0xff777777);
	}
}

void ScriptContentComponent::updateValue(int i)
{
	MacroControlledObject *o = dynamic_cast<MacroControlledObject*>(componentWrappers[i]->getComponent());

	if (o != nullptr)
	{
		o->updateValue(dontSendNotification);
	}

	if (TableEditor *t = dynamic_cast<TableEditor*>(componentWrappers[i]->getComponent()))
	{
		t->setDisplayedIndex((float)contentData->components[i]->value / 127.0f);
	}

	if (Slider *s = dynamic_cast<Slider*>(componentWrappers[i]->getComponent()))
	{
		if (s->getSliderStyle() == Slider::TwoValueHorizontal)
		{
			const double min = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(contentData->components[i].get())->getMinValue();
			const double max = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(contentData->components[i].get())->getMaxValue();

			s->setMinValue(min, dontSendNotification);
			s->setMaxValue(max, dontSendNotification);
		}
	}
}

void ScriptContentComponent::updateValues()
{
	for (int i = 0; i < componentWrappers.size(); i++)
	{
		updateValue(i);
	}
}

void ScriptContentComponent::changeListenerCallback(SafeChangeBroadcaster *b)
{
	if (contentData.get() == nullptr) return;

	if (p.get() == nullptr)
	{
		setEnabled(false);
	}

	if (p == b)
	{
#if USE_BACKEND
		updateValues();
#endif
	}
	else if (dynamic_cast<ScriptingApi::Content::ScriptComponent*>(b) != nullptr)
	{
		updateContent(dynamic_cast<ScriptingApi::Content::ScriptComponent*>(b));
	}
	else
	{
		updateContent();
	}
}

void ScriptContentComponent::updateComponent(int i)
{
    if(componentWrappers[i]->getComponent() == nullptr)
    {
        jassertfalse;
        return;
    }
    
	

	const bool v = contentData->components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::visible);
	componentWrappers[i]->getComponent()->setVisible(v);

	if (!contentData->components[i]->isShowing())
	{
		return;
	}

	const bool e = contentData->components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::enabled);
	componentWrappers[i]->getComponent()->setEnabled(e);

	const Rectangle<int> localBounds = componentWrappers[i]->getComponent()->getBoundsInParent();
	const Rectangle<int> currentPosition = contentData->components[i]->getPosition();
	
	auto currentParentId = componentWrappers[i]->getComponent()->getParentComponent()->getName();
	auto newParentId = contentData->components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::parentComponent).toString();

	if (currentParentId != newParentId)
	{
		auto componentToRemove = componentWrappers[i]->getComponent();

		if (newParentId.isEmpty())
		{
			componentToRemove->getParentComponent()->removeChildComponent(componentToRemove);

			addChildComponent(componentToRemove);
			componentToRemove->setBounds(currentPosition);
		}
		else
		{
			for (int c = 0; c < componentWrappers.size(); c++)
			{
				if (componentWrappers[c]->getComponent()->getName() == newParentId)
				{
					auto newParent = componentWrappers[c]->getComponent();
					componentToRemove->getParentComponent()->removeChildComponent(componentToRemove);
					newParent->addChildComponent(componentToRemove);
					componentToRemove->setBounds(currentPosition);
					break;
				}
			}
		}

		
	}

	if (localBounds != currentPosition)
	{
		const bool isInViewport = dynamic_cast<Viewport*>(componentWrappers[i]->getComponent()->getParentComponent()) != nullptr;
		const bool sizeChanged = localBounds.getWidth() != currentPosition.getWidth() || localBounds.getHeight() != currentPosition.getHeight();

		if (!isInViewport || sizeChanged)
			componentWrappers[i]->getComponent()->setBounds(contentData->components[i]->getPosition());
	}

	componentWrappers[i]->updateComponent();
	updateValue(i);
}

void ScriptContentComponent::updateContent(ScriptingApi::Content::ScriptComponent* componentToUpdate/*=nullptr*/)
{
	if (contentData.get() == nullptr) return;

	if (componentToUpdate == nullptr)
	{

		jassert(contentData->components.size() == componentWrappers.size());

		for (int i = 0; i < contentData->components.size(); i++)
		{
			updateComponent(i);
		}

		//resized();
	}
	else
	{
		jassert(contentData->components.size() == componentWrappers.size());

		auto index = contentData->components.indexOf(componentToUpdate);

		if (index >= 0)
			updateComponent(index);
		else
			jassertfalse;
	}
}

void ScriptContentComponent::resized()
{
	if (!contentValid())
	{
		return;
	}

	for (int i = 0; i < componentWrappers.size(); i++)
	{
		Component *c = componentWrappers[i]->getComponent();

		if (c->getLocalBounds() != contentData->components[i]->getPosition())
		{
			c->setBounds(contentData->components[i]->getPosition());
		}
	}
}

void ScriptContentComponent::scriptWasCompiled(JavascriptProcessor *jp)
{
	if (jp == getScriptProcessor())
	{
		setNewContent(processor->getScriptingContent());
		updateContent();
	}
}

void ScriptContentComponent::setNewContent(ScriptingApi::Content *c)
{
	if (c == nullptr) return;

	contentData = c;

	contentData->addChangeListener(this);

	deleteAllScriptComponents();

	for (int i = 0; i < contentData->components.size(); i++)
	{
		auto sc = contentData->components[i].get();
		
		sc->addChangeListener(this);

		componentWrappers.add(sc->createComponentWrapper(this, i));

		const int parentIndex = sc->getParentComponentIndex();

		if (parentIndex != -1 && parentIndex < componentWrappers.size())
		{
			Component* parentComponentOfSc = componentWrappers[parentIndex]->getComponent();
			
			if (parentComponentOfSc != nullptr)
			{
				if (auto vp = dynamic_cast<Viewport*>(parentComponentOfSc))
				{
					vp->setViewedComponent(componentWrappers.getLast()->getComponent(), false);
				}
				else
				{
					parentComponentOfSc->addAndMakeVisible(componentWrappers.getLast()->getComponent());
				}
			}
		}
		else
		{
			addAndMakeVisible(componentWrappers.getLast()->getComponent());
		}
	}

	refreshMacroIndexes();
	refreshContentButton();

	if (getWidth() != 0) setSize(getWidth(), getContentHeight());

	updateContent();
    
    addMouseListenersForComponentWrappers();
}

void ScriptContentComponent::addMouseListenersForComponentWrappers()
{
    if (getParentComponent() != nullptr)
    {
        for (int i = 0; i < componentWrappers.size(); i++)
        {
            componentWrappers[i]->getComponent()->addMouseListener(getParentComponent(), true);
        }
    }
}

void ScriptContentComponent::refreshContentButton()
{
#if USE_BACKEND

	if(ScriptingEditor *e = dynamic_cast<ScriptingEditor*>(getParentComponent()))
	{
		e->checkContent();
	}

#endif

}


ScriptingApi::Content::ScriptComponent * ScriptContentComponent::getEditedComponent()
{
	if (contentData.get() != nullptr && editedComponent != -1)
	{
		return contentData->getComponent(editedComponent);
	}

	return nullptr;
}

Component* ScriptContentComponent::setEditedScriptComponent(ScriptingApi::Content::ScriptComponent *sc)
{
	if (sc == nullptr)
	{
		editedComponent = -1;

		setWantsKeyboardFocus(false);

		repaint();
		return nullptr;

	}

	String id = sc->getName().toString();

	// Use the text value for the slider (hack because the slider class doesn't allow a dedicated name. */
	String text = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::text).toString();

	for (int i = 0; i < componentWrappers.size(); i++)
	{

		const bool isSlider = dynamic_cast<Slider*>(componentWrappers[i]->getComponent()) != nullptr;

		const bool sliderMatches = isSlider && (componentWrappers[i]->getComponent()->getName() == text || componentWrappers[i]->getComponent()->getName() == id);

		if (sliderMatches || id == componentWrappers[i]->getComponent()->getName())
		{
			editedComponent = i;

			//setWantsKeyboardFocus(true);
			//grabKeyboardFocus();

			repaint();
			return componentWrappers[i]->getComponent();
		}
	}

	editedComponent = -1;

	repaint();

	return nullptr;
}

bool ScriptContentComponent::keyPressed(const KeyPress &/*key*/)
{
	return false;
}

void ScriptContentComponent::paint(Graphics &g)
{
#if USE_BACKEND
	if(dynamic_cast<ScriptingEditor*>(getParentComponent()) != nullptr)
	{
		g.fillAll(Colours::white.withAlpha(0.05f));
		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.1f), 0.0f, 0.0f,
			Colours::transparentBlack, 0.0f, 6.0f, false));
		g.fillRect(0.0f, 0.0f, (float)getWidth(), 6.0f);
	}
#else
	ignoreUnused(g);
#endif
}

ScriptingApi::Content::ScriptComponent * ScriptContentComponent::getScriptComponentFor(Point<int> pos)
{
	for (int i = componentWrappers.size() - 1; i >= 0; --i)
	{
		Component* c = componentWrappers[i]->getComponent();

		if (!c->isVisible()) continue;

		Component* parentOfC = c->getParentComponent();

		if (getLocalArea(parentOfC, c->getBounds()).contains(pos))
		{
			return contentData->getComponent(i);
		}
	}

	return nullptr;
}

ScriptingApi::Content::ScriptComponent* ScriptContentComponent::getScriptComponentFor(Component* component)
{
	for (int i = 0; i < componentWrappers.size(); i++)
	{
		if (contentData.get() != nullptr && componentWrappers[i]->getComponent() == component)
		{
			return contentData->getComponent(i);
		}
	}

	return nullptr;
}

void ScriptContentComponent::getScriptComponentsFor(Array<ScriptingApi::Content::ScriptComponent*> &arrayToFill, Point<int> pos)
{
	for (int i = componentWrappers.size() - 1; i >= 0; --i)
	{
		Component* c = componentWrappers[i]->getComponent();

		Component* parentOfC = c->getParentComponent();

		if (getLocalArea(parentOfC, c->getBounds()).contains(pos))
		{
			arrayToFill.add(contentData->getComponent(i));
		}
	}
}
