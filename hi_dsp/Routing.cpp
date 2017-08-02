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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#include "Routing.h"

RoutableProcessor::MatrixData::MatrixData(RoutableProcessor *p) :
owningProcessor(p),
numSourceChannels(2),
numDestinationChannels(2),
resizeAllowed(false),
allowEnablingOnly(false),
editorShown(false)
{
	
    
    
	resetToDefault();
}

void RoutableProcessor::MatrixData::clearAllConnections()
{
	for (int i = 0; i < NUM_MAX_CHANNELS; i++)
	{
		channelConnections[i] = -1;
		sendConnections[i] = -1;
	}

	refreshSourceUseStates();
}

bool RoutableProcessor::MatrixData::isUsed(int sourceChannel) const noexcept
{
	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return false;
	}

	return channelConnections[sourceChannel] != -1 || sendConnections[sourceChannel] != -1;
}

bool RoutableProcessor::MatrixData::toggleEnabling(int sourceChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return false;
	}

	if (channelConnections[sourceChannel] == sourceChannel)
	{
		removeConnection(sourceChannel, sourceChannel);
	}
	else
	{
		addConnection(sourceChannel, sourceChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::toggleSendEnabling(int sourceChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return false;
	}

	if (channelConnections[sourceChannel] == sourceChannel)
	{
		removeSendConnection(sourceChannel, sourceChannel);
	}
	else
	{
		addSendConnection(sourceChannel, sourceChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::toggleConnection(int sourceChannel, int destinationChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	if (channelConnections[sourceChannel] == destinationChannel)
	{
		removeConnection(sourceChannel, destinationChannel);
	}
	else
	{
		addConnection(sourceChannel, destinationChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::addConnection(int sourceChannel, int destinationChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}
	
	channelConnections[sourceChannel] = destinationChannel;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (channelConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections > 2)
		{
			const bool removeOldEvenConnection = sourceChannel % 2 == 0;

			for (int i = removeOldEvenConnection ? 0 : 1; i < getNumSourceChannels(); i += 2)
			{
				if (i == sourceChannel) continue;
				channelConnections[i] = -1;
			}
		}
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::removeConnection(int sourceChannel, int destinationChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	channelConnections[sourceChannel] = -1;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (channelConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections < 2)
		{
			const int index = sourceChannel % 2 != 0 ? 1 : 0;

			channelConnections[index] = index;
		}
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::toggleSendConnection(int sourceChannel, int destinationChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	if (sendConnections[sourceChannel] == destinationChannel)
	{
		removeSendConnection(sourceChannel, destinationChannel);
	}
	else
	{
		addSendConnection(sourceChannel, destinationChannel);
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::addSendConnection(int sourceChannel, int destinationChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	sendConnections[sourceChannel] = destinationChannel;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (sendConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections > 2)
		{
			const bool removeOldEvenConnection = sourceChannel % 2 == 0;

			for (int i = removeOldEvenConnection ? 0 : 1; i < getNumSourceChannels(); i += 2)
			{
				if (i == sourceChannel) continue;
				sendConnections[i] = -1;
			}
		}
	}

	refreshSourceUseStates();

	return true;
}

bool RoutableProcessor::MatrixData::removeSendConnection(int sourceChannel, int destinationChannel) noexcept
{
	ScopedLock sl(getLock());

	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels() || destinationChannel < 0 || destinationChannel >= getNumDestinationChannels())
	{
		return false;
	}

	sendConnections[sourceChannel] = -1;

	if (numAllowedConnections == 2)
	{
		int numConnections = 0;

		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			if (sendConnections[i] != -1)
			{
				numConnections++;
			}
		}

		if (numConnections < 2)
		{
			const int index = sourceChannel % 2 != 0 ? 1 : 0;

			sendConnections[index] = index;
		}
	}

	refreshSourceUseStates();

	return true;
}

int RoutableProcessor::MatrixData::getConnectionForSourceChannel(int sourceChannel) const noexcept
{
	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return -1;
	}

	const int destination = channelConnections[sourceChannel];

	if (destination < numDestinationChannels)
	{
		return destination;
	}
	else return -1;
}

int RoutableProcessor::MatrixData::getSendForSourceChannel(int sourceChannel) const noexcept
{
	if (sourceChannel < 0 || sourceChannel >= getNumSourceChannels())
	{
		return -1;
	}

	const int destination = sendConnections[sourceChannel];

	if (destination < numDestinationChannels)
	{
		return destination;
	}
	else return -1;
}

void RoutableProcessor::MatrixData::resetToDefault()
{
	for (int i = 0; i < NUM_MAX_CHANNELS; i++)
	{
		channelConnections[i] = -1;
		sendConnections[i] = -1;
	}

	channelConnections[0] = 0;
	channelConnections[1] = 1;
    
    FloatVectorOperations::clear(targetGainValues, NUM_MAX_CHANNELS);
    FloatVectorOperations::clear(sourceGainValues, NUM_MAX_CHANNELS);

	refreshSourceUseStates();
}

ValueTree RoutableProcessor::MatrixData::exportAsValueTree() const
{
	ValueTree v("RoutingMatrix");

	v.setProperty("NumSourceChannels", numSourceChannels, nullptr);

	for (int i = 0; i < getNumSourceChannels(); i++)
	{
		v.setProperty("Channel" + String(i), channelConnections[i], nullptr);
		v.setProperty("Send" + String(i), sendConnections[i], nullptr);
	}
    
	return v;
}

void RoutableProcessor::MatrixData::restoreFromValueTree(const ValueTree &v)
{
	if (!(v.getType() == Identifier("RoutingMatrix"))) return;

	clearAllConnections();

	setNumSourceChannels(v.getProperty("NumSourceChannels", 2));

	for (int i = 0; i < getNumSourceChannels(); i++)
	{
		Identifier id = "Channel" + String(i);

		if (v.hasProperty(id))
		{
			const int dest = (int)v.getProperty(id, -1);

			channelConnections[i] = dest;
		}

		Identifier id2 = "Send" + String(i);

		if (v.hasProperty(id2))
		{
			const int dest = (int)v.getProperty(id2, -1);

			sendConnections[i] = dest;
		}
	}

	refreshSourceUseStates();
}

void RoutableProcessor::MatrixData::setNumSourceChannels(int newNumChannels, NotificationType notifyProcessors)
{
	jassert(newNumChannels <= NUM_MAX_CHANNELS);

	newNumChannels = jmin<int>(newNumChannels, NUM_MAX_CHANNELS);

	if (newNumChannels != numSourceChannels)
	{
		ScopedLock sl(getLock());

		numSourceChannels = jmax<int>(1, newNumChannels);
		//clearAllConnections();

		refreshSourceUseStates();

		if (notifyProcessors == sendNotification) owningProcessor->numSourceChannelsChanged();
	}
}

void RoutableProcessor::MatrixData::setNumDestinationChannels(int newNumChannels, NotificationType notifyProcessors)
{
	jassert(newNumChannels <= NUM_MAX_CHANNELS);

	newNumChannels = jmin<int>(newNumChannels, NUM_MAX_CHANNELS);

	if (newNumChannels != numDestinationChannels)
	{
		ScopedLock sl(getLock());

		numDestinationChannels = jmax<int>(1, newNumChannels);
		refreshSourceUseStates();
	}

	if (notifyProcessors == sendNotification) owningProcessor->numDestinationChannelsChanged();
}

const CriticalSection & RoutableProcessor::MatrixData::getLock() const
{
	return thisAsProcessor->getMainController()->getLock();
}

void RoutableProcessor::MatrixData::setTargetProcessor(Processor *p)
{
	targetProcessor = p;
}

void RoutableProcessor::MatrixData::setGainValues(float *numMaxChannelValues, bool isSourceValue)
{
	ScopedLock sl(getLock());

	memcpy(isSourceValue ? sourceGainValues : targetGainValues, numMaxChannelValues, (isSourceValue ? numSourceChannels : numDestinationChannels) * sizeof(float));
}

void RoutableProcessor::MatrixData::loadPreset(Presets newPreset)
{
	Presets pr = (Presets)newPreset;

	clearAllConnections();

	switch (pr)
	{
	case Presets::AllChannels:
		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			addConnection(i, i);
		}
		break;
	case Presets::FirstStereo:
		addConnection(0, 0);
		addConnection(1, 1);
		break;
	case Presets::SecondStereo:
		addConnection(2, 2);
		addConnection(3, 3);

		break;
	case Presets::ThirdStereo:
		addConnection(4, 4);
		addConnection(5, 5);

		break;
	case Presets::AllChannelsToStereo:
		for (int i = 0; i < getNumSourceChannels(); i++)
		{
			addConnection(i, i % 2 != 0 ? 1 : 0);
		}
		break;
	default:
		break;
	}
}

void RoutableProcessor::MatrixData::refreshSourceUseStates()
{
	for (int i = 0; i < numSourceChannels; i++)
	{
		if (channelConnections[i] != -1)
		{
			owningProcessor->leftSourceChannel = i;
			owningProcessor->leftTargetChannel = channelConnections[i];
			break;
		}

	}

	for (int i = numSourceChannels - 1; i >= 0; i--)
	{
		if (channelConnections[i] != -1)
		{
			owningProcessor->rightSourceChannel = i;
			owningProcessor->rightTargetChannel = channelConnections[i];
			break;
		}
	}

	owningProcessor->connectionChanged();

	sendChangeMessage();
}


RoutableProcessor::RoutableProcessor() :
data(this),
leftSourceChannel(-1),
rightSourceChannel(-1),
leftTargetChannel(-1),
rightTargetChannel(-1)
{

}

void RoutableProcessor::editRouting(Component *childComponent)
{
#if USE_BACKEND

	AlertWindowLookAndFeel laf;

	auto editor = GET_BACKEND_ROOT_WINDOW(childComponent);

	if (editor != nullptr)
	{
		String id = dynamic_cast<Processor*>(this)->getId();

		editor->getRootFloatingTile()->showComponentInRootPopup(new RouterComponent(&getMatrix()), childComponent, childComponent->getLocalBounds().getCentre());

	}
#else 

	ignoreUnused(childComponent);

#endif
	
}