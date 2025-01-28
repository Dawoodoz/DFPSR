#!/bin/bash

# TODO: Give whole folders and let the application search for the files using the file API.

# Call the application using a list of model generation scripts to pre-render graphics.
#   Ortho.ini defines the game's camera system and should be used by the game when drawing the generated sprites.
../Sandbox ./models ./images ./Ortho.ini Floor WoodenFloor WoodenFence WoodenBarrel Pillar Character_Mage

