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

RouteEffect::RouteEffect(MainController *mc, const String &uid) :
MasterEffectProcessor(mc, uid)
{
	getMatrix().setOnlyEnablingAllowed(false);
}

ProcessorEditorBody *RouteEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new RouteFXEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

void RouteEffect::renderWholeBuffer(AudioSampleBuffer &b)
{
	

	const int numSamples = b.getNumSamples();

	if (getMatrix().isEditorShown())
	{
		float gainValues[NUM_MAX_CHANNELS];

		jassert(getMatrix().getNumSourceChannels() == b.getNumChannels());

		for (int i = 0; i < b.getNumChannels(); i++)
		{
			gainValues[i] = b.getMagnitude(i, 0, b.getNumSamples());
		}

		getMatrix().setGainValues(gainValues, true);
	}

	for (int i = 0; i < b.getNumChannels(); i++)
	{
		const int j = getMatrix().getSendForSourceChannel(i);

		if (j != -1)
		{
			FloatVectorOperations::add(b.getWritePointer(j), b.getReadPointer(i), numSamples);
		}
	}

	if (getMatrix().isEditorShown())
	{
		float gainValues[NUM_MAX_CHANNELS];

		jassert(getMatrix().getNumDestinationChannels() == b.getNumChannels());

		for (int i = 0; i < b.getNumChannels(); i++)
		{
			gainValues[i] = b.getMagnitude(i, 0, b.getNumSamples());
		}

		getMatrix().setGainValues(gainValues, false);
	}
	

}

void RouteEffect::applyEffect(AudioSampleBuffer &, int, int /*numSamples*/)
{
	
}

