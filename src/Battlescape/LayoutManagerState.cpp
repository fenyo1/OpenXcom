/*
 * Copyright 2010-2013 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "LayoutManagerState.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "Camera.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Action.h"
#include "../Engine/Timer.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ArrowButton.h"
#include "../Resource/ResourcePack.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/RuleInventory.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Tile.h"
#include "../Savegame/EquipmentLayout.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "WarningMessage.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Layout Manager screen.
 * @param game Pointer to the core game.
 */
LayoutManagerState::LayoutManagerState(Game *game, bool isBattlescapeGame) : State(game), _selectedLayout(0), _btnLayoutsStartIndex(0)
{
	_save = _game->getSavedGame();
	_battleGame = _save->getSavedBattle();

	int tileWidth = 0, tileHeight = 0;
	if (isBattlescapeGame)
	{
		Surface *blankPck = _game->getResourcePack()->getSurfaceSet("BLANKS.PCK")->getFrame(0);
		tileWidth = blankPck->getWidth();
		tileHeight = blankPck->getHeight();
	}

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(36, 12, 276, 8);
	_txtTitle = new Text(310, 17, 5, 5);
	_txtMode = new Text(310, 9, 5, 24);
	_btnToSoldier = new ToggleTextButton(310, 14, 5, 32);
	_btnFromSoldier = new ToggleTextButton(310, 14, 5, 46);
	_txtSelectedLayout = new Text(99, 9, 5, 64);
	_btnLayoutsUp = new ArrowButton(ARROW_BIG_UP, 13, 14, 86, 72);
	_btnLayoutsDown = new ArrowButton(ARROW_BIG_DOWN, 13, 14, 86, 72 + 6*14 - 14);
	_txtSoldiers = new Text(114, 9, 108, 64);
	_txtName = new Text(114, 9, 108, 72);
	_txtLayout = new Text(75, 9, 222, 72);
	_lstSoldiers = new TextList(197, 72, 104, 80);
	_txtLayoutName = new Text(135, 9, 5, 158);
	_edtLayout = new TextEdit(135, 16, 5, 167);
	_btnCreate = new TextButton(60, 12, 5, 183);
	_btnRename = new TextButton(60, 12, 67, 183);
	_btnDelete = new TextButton(60, 12, 129, 183);
	if (isBattlescapeGame)
	{
		_txtOutsideBrightness = new Text(60, 18, 191, 158);
		_outsideBrightness = new Surface(tileWidth * 2, tileHeight * 1, 0, 0);
	}
	_warning = new WarningMessage(224, 24, 48, 176);

	add(_window);
	add(_txtTitle);
	add(_btnOk);
	add(_txtMode);
	add(_btnToSoldier);
	add(_btnFromSoldier);
	add(_txtSelectedLayout);
	add(_btnLayoutsUp);
	add(_btnLayoutsDown);
	add(_txtSoldiers);
	add(_txtName);
	add(_txtLayout);
	add(_txtLayoutName);
	add(_btnCreate);
	add(_btnRename);
	add(_btnDelete);
	add(_lstSoldiers);
	add(_edtLayout);
	if (isBattlescapeGame)
	{
		add(_txtOutsideBrightness);
		add(_outsideBrightness);
	}
	add(_warning);

	centerAllSurfaces();

	// Set up objects
	_window->setColor(Palette::blockOffset(1)-4);

	_txtTitle->setColor(Palette::blockOffset(5));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_LAYOUT_MANAGER"));
	_txtTitle->setHighContrast(true);

	_btnOk->setColor(Palette::blockOffset(5));
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&LayoutManagerState::btnOkClick);
	_btnOk->setHighContrast(true);

	_txtMode->setColor(Palette::blockOffset(3));
	_txtMode->setText(tr("STR_MODE_UC"));
	_txtMode->setHighContrast(true);

	_btnToSoldier->setColor(Palette::blockOffset(0)+4);
	_btnToSoldier->setInvertColor(Palette::blockOffset(1)+5);
	_btnToSoldier->onMousePress((ActionHandler)&LayoutManagerState::btnModeXPress, 0);
	_btnToSoldier->setText(tr("STR_LAYOUT_TO_SOLDIER"));
	_btnToSoldier->setPressed(true);

	_btnFromSoldier->setColor(Palette::blockOffset(0)+4);
	_btnFromSoldier->setInvertColor(Palette::blockOffset(1)+5);
	_btnFromSoldier->onMousePress((ActionHandler)&LayoutManagerState::btnModeXPress, 0);
	_btnFromSoldier->setText(tr("STR_LAYOUT_FROM_SOLDIER"));

	_txtSelectedLayout->setColor(Palette::blockOffset(3));
	_txtSelectedLayout->setText(tr("STR_SELECTED_LAYOUT_UC"));
	_txtSelectedLayout->setHighContrast(true);

	std::wstringstream ss;
	ss << tr("STR_SOLDIERS_UC") << ">";
	_txtSoldiers->setColor(Palette::blockOffset(3));
	_txtSoldiers->setText(ss.str());
	_txtSoldiers->setHighContrast(true);

	_txtName->setColor(Palette::blockOffset(11)-1);
	_txtName->setText(tr("STR_NAME_UC"));
	_txtName->setHighContrast(true);

	_txtLayout->setColor(Palette::blockOffset(11)-1);
	_txtLayout->setText(tr("STR_LAYOUT_UC"));
	_txtLayout->setHighContrast(true);

	_txtLayoutName->setColor(Palette::blockOffset(3));
	_txtLayoutName->setText(tr("STR_LAYOUT_NAME_UC"));
	_txtLayoutName->setHighContrast(true);

	_btnCreate->setColor(Palette::blockOffset(5));
	_btnCreate->setText(tr("STR_CREATE_UC"));
	_btnCreate->onMouseClick((ActionHandler)&LayoutManagerState::btnCreateClick);
	_btnCreate->setHighContrast(true);

	_btnRename->setColor(Palette::blockOffset(5));
	_btnRename->setText(tr("STR_RENAME_UC"));
	_btnRename->onMouseClick((ActionHandler)&LayoutManagerState::btnRenameClick);
	_btnRename->setHighContrast(true);
	_btnRename->setVisible(false);

	_btnDelete->setColor(Palette::blockOffset(5));
	_btnDelete->setText(tr("STR_DELETE"));
	_btnDelete->onMouseClick((ActionHandler)&LayoutManagerState::btnDeleteClick);
	_btnDelete->setHighContrast(true);
	_btnDelete->setVisible(false);

	_lstSoldiers->setColor(Palette::blockOffset(13)-1);
	_lstSoldiers->setArrowColor(Palette::blockOffset(13)+2);
	_lstSoldiers->setColumns(2, 114, 75);
	_lstSoldiers->setScrolling(true, 1);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(4);
	_lstSoldiers->setHighContrast(true);
	_lstSoldiers->onMousePress((ActionHandler)&LayoutManagerState::lstSoldiersPress);

	for (int buttons = 0; buttons <= _save->getLayouts()->size() && buttons < 6; ) addLayoutButton(L"", buttons);
	_btnLayoutsUp->setColor(Palette::blockOffset(0)+4);
	_btnLayoutsUp->onMousePress((ActionHandler)&LayoutManagerState::btnLayoutsUpPress);
	_btnLayoutsUp->onMouseRelease((ActionHandler)&LayoutManagerState::btnLayoutsUpRelease);
	_btnLayoutsUp->onMouseClick((ActionHandler)&LayoutManagerState::btnLayoutsUpClick, 0);
	_btnLayoutsUp->setVisible(false);
	_btnLayoutsDown->setColor(Palette::blockOffset(0)+4);
	_btnLayoutsDown->onMousePress((ActionHandler)&LayoutManagerState::btnLayoutsDownPress);
	_btnLayoutsDown->onMouseRelease((ActionHandler)&LayoutManagerState::btnLayoutsDownRelease);
	_btnLayoutsDown->onMouseClick((ActionHandler)&LayoutManagerState::btnLayoutsDownClick, 0);
	_btnLayoutsDown->setVisible(true);
	_timerLayoutsUp = new Timer(250);
	_timerLayoutsDown = new Timer(250);
	_timerLayoutsUp->onTimer((StateHandler)&LayoutManagerState::onTimerLayoutsUp);
	_timerLayoutsDown->onTimer((StateHandler)&LayoutManagerState::onTimerLayoutsDown);
	updateLayoutsButtons();

	EquipmentLayout *layout = (_save->getNewSoldierLayoutId() == 0) ? 0 : _save->getLayout(_save->getNewSoldierLayoutId());
	_lstSoldiers->addRow(2, tr("STR_NEWLY_RECRUITED_SOLDIERS").c_str(), (layout == 0 || layout->getId() == 0) ? tr("STR_CUSTOM").c_str() : layout->getName().c_str());
	_lstSoldiers->setRowColor(0, Palette::blockOffset(16)-2);
	for (std::vector<BattleUnit*>::iterator i = _battleGame->getUnits()->begin(); i != _battleGame->getUnits()->end(); ++i)
	{
		// we need X-Com soldiers only
		if ((*i)->getGeoscapeSoldier() == 0) continue;

		_soldiers.push_back(*i);
		layout = (*i)->getGeoscapeSoldier()->getEquipmentLayout();
		_lstSoldiers->addRow(2, (*i)->getName(0).c_str(), (layout == 0 || layout->getId() == 0) ? tr("STR_CUSTOM").c_str() : layout->getName().c_str());
	}

	_edtLayout->setColor(Palette::blockOffset(1)-2);
	_edtLayout->setBig();
	_edtLayout->setText(_btnLayouts.at(0)->getText());
	_edtLayout->setHighContrast(true);
	_edtLayout->onKeyboardPress((ActionHandler)&LayoutManagerState::edtLayoutKeyPress);

	if (isBattlescapeGame)
	{
		_txtOutsideBrightness->setColor(Palette::blockOffset(3));
		_txtOutsideBrightness->setText(tr("STR_OUTSIDE_BRIGHTNESS_UC"));
		_txtOutsideBrightness->setWordWrap(true);
		_txtOutsideBrightness->setHighContrast(true);

		Tile *tile = 0;
		Surface *tmpSurface = 0;
		int endX = _battleGame->getMapSizeX();
		int endY = _battleGame->getMapSizeY();
		for (int itX = 0; tmpSurface == 0 && itX < endX; ++itX)
		{
			for (int itY = 0; tmpSurface == 0 && itY < endY; ++itY)
			{
				tile = _battleGame->getTile(Position(itX, itY, 0));
				if (tile) tmpSurface = tile->getSprite(MapData::O_FLOOR);
			}
		}
		int tileShade = _battleGame->getGlobalShade();
		Camera *cam = _battleGame->getBattleState()->getMap()->getCamera();
		Position screenPosition;
		if (tmpSurface)
		for (int itX = 0; itX < 2; ++itX)
		{
			for (int itY = 0; itY > -2; --itY)
			{
				cam->convertMapToScreen(Position(itX, itY, 0), &screenPosition);
				tmpSurface->blitNShade(_outsideBrightness, screenPosition.x, screenPosition.y - tileHeight/4, tileShade, false);
			}
		}
		cam->convertMapToScreen(Position(1, 0, 0), &screenPosition);
		SurfaceSet *cursorPck = _game->getResourcePack()->getSurfaceSet("CURSOR.PCK");
		cursorPck->getFrame(0)->blitNShade(_outsideBrightness, screenPosition.x, screenPosition.y - tileHeight/4, 0);
		cursorPck->getFrame(3)->blitNShade(_outsideBrightness, screenPosition.x, screenPosition.y - tileHeight/4, 0);
		_outsideBrightness->setX(251);
		_outsideBrightness->setY(157);
	}

	_warning->initText(_game->getResourcePack()->getFont("FONT_BIG"), _game->getResourcePack()->getFont("FONT_SMALL"), _game->getLanguage());
	_warning->setColor(Palette::blockOffset(2));
	_warning->setTextColor(Palette::blockOffset(1)-1);
}

/**
 *
 */
LayoutManagerState::~LayoutManagerState()
{
	delete _timerLayoutsUp;
	delete _timerLayoutsDown;
}

/**
 * Creates and adds a LayoutButton.
 * @param text Text of the button.
 * @param buttons index of the button.
 */
void LayoutManagerState::addLayoutButton(const std::wstring &text, int &buttons)
{
	ToggleTextButton *btnLayout;
	btnLayout = new ToggleTextButton(80, 14, 5, 72 + buttons*14);
	btnLayout->setColor(Palette::blockOffset(0)+4);
	btnLayout->setInvertColor(Palette::blockOffset(1)-3);
	btnLayout->onMousePress((ActionHandler)&LayoutManagerState::btnLayoutXPress, 0);
	btnLayout->setText(text);
	_btnLayouts.push_back(btnLayout);
	add(btnLayout);
	++buttons;
}

/**
 * Goes back to the Inventory screen.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Occurs when the player clicks on a mode-button.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnModeXPress(Action *action)
{
	if (action->getDetails()->button.button != SDL_BUTTON_LEFT
	&&  action->getDetails()->button.button != SDL_BUTTON_RIGHT) return;
	_btnToSoldier->setPressed(action->getSender() == _btnToSoldier);
	_btnFromSoldier->setPressed(action->getSender() == _btnFromSoldier);
}

/**
 * Occurs when the player clicks on a soldier-button.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnLayoutXPress(Action *action)
{
	if (action->getDetails()->button.button != SDL_BUTTON_LEFT
	&&  action->getDetails()->button.button != SDL_BUTTON_RIGHT) return;
	int buttonIndex = (action->getSender()->getY() - 72) / 14;
	_selectedLayout = _btnLayoutsStartIndex + buttonIndex;
	for (int i = 0; i < _btnLayouts.size(); ++i)
	{
		_btnLayouts.at(i)->setPressed(i == buttonIndex);
	}
	_edtLayout->setText(_btnLayouts.at(buttonIndex)->getText());
	updateEditButtons();
}

/**
 * Handler for pressing the up arrowbutton.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnLayoutsUpPress(Action * action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT) _timerLayoutsUp->start();
}

/**
 * Handler for releasing the up arrowbutton.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnLayoutsUpRelease(Action * action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLayoutsUp->setInterval(250);
		_timerLayoutsUp->stop();
	}
}

/**
 * Handler for clicking the up arrowbutton.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnLayoutsUpClick(Action * action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) scrollLayoutsUp(std::numeric_limits<int>::max());
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT) scrollLayoutsUp(1);
}

/**
 * Handler for pressing the down arrowbutton.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnLayoutsDownPress(Action * action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT) _timerLayoutsDown->start();
}

/**
 * Handler for releasing the down arrowbutton.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnLayoutsDownRelease(Action * action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLayoutsDown->setInterval(250);
		_timerLayoutsDown->stop();
	}
}

/**
 * Handler for clicking the down arrowbutton.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnLayoutsDownClick(Action * action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) scrollLayoutsDown(std::numeric_limits<int>::max());
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT) scrollLayoutsDown(1);
}

/**
 * Event handler for _timerLayoutsUp.
 */
void LayoutManagerState::onTimerLayoutsUp()
{
	_timerLayoutsUp->setInterval(50);
	scrollLayoutsUp(1);
}

/**
 * Event handler for _timerLayoutsDown.
 */
void LayoutManagerState::onTimerLayoutsDown()
{
	_timerLayoutsDown->setInterval(50);
	scrollLayoutsDown(1);
}

/**
 * Scrolls up by the number of change.
 * @param change The number of lines to scroll up.
 */
void LayoutManagerState::scrollLayoutsUp(int change)
{
	if (change <= 0 || _btnLayoutsStartIndex == 0) return;
	if (change > _btnLayoutsStartIndex) _btnLayoutsStartIndex = 0; else _btnLayoutsStartIndex -= change;
	updateLayoutsButtons();
}

/**
 * Scrolls down by the number of change.
 * @param change The number of lines to scroll down.
 */
void LayoutManagerState::scrollLayoutsDown(int change)
{
	int max = ((int) _save->getLayouts()->size()) - 5;
	if (change <= 0 || _btnLayoutsStartIndex == max) return;
	// we need to check 'change' alone first, because _btnLayoutsStartIndex+change can overflow
	if (change > max || _btnLayoutsStartIndex+change > max) _btnLayoutsStartIndex = max; else _btnLayoutsStartIndex += change;
	updateLayoutsButtons();
}

/**
 * Updates the layout buttons, and arrowButtons.
 */
void LayoutManagerState::updateLayoutsButtons()
{
	int buttonIndex = _selectedLayout - _btnLayoutsStartIndex;
	for (int i = 0; i < _btnLayouts.size(); ++i)
	{
		_btnLayouts.at(i)->setText((_btnLayoutsStartIndex+i == 0) ? tr("STR_CUSTOM") : _save->getLayouts()->at(_btnLayoutsStartIndex+i-1)->getName());
		_btnLayouts.at(i)->setPressed(i == buttonIndex);
	}
	int max = ((int) _save->getLayouts()->size()) - 5;
	bool b;
	b = (_btnLayoutsStartIndex > 0);
	if (_btnLayoutsUp->getVisible() && !b) _btnLayoutsUp->unpress(this); // This is a workaround to avoid a crash!
	_btnLayoutsUp->setVisible(b);
	b = (_btnLayoutsStartIndex < max);
	if (_btnLayoutsDown->getVisible() && !b) _btnLayoutsDown->unpress(this); // This is a workaround to avoid a crash!
	_btnLayoutsDown->setVisible(b);
}

/**
 * Updates the visibility of Create, Rename, and Delete layout buttons.
 */
void LayoutManagerState::updateEditButtons()
{
	_btnCreate->setVisible(!_edtLayout->getText().empty());
	_btnRename->setVisible(_selectedLayout != 0 && !_edtLayout->getText().empty());
	_btnDelete->setVisible(_selectedLayout != 0);
}

/**
 * Runs state functionality every cycle.
 */
void LayoutManagerState::think()
{
	State::think();
	if (_timerLayoutsUp) _timerLayoutsUp->think(this, 0);
	if (_timerLayoutsDown) _timerLayoutsDown->think(this, 0);
}

/**
 * Occurs when the player presses on a soldier.
 * @param action Pointer to an action.
 */
void LayoutManagerState::lstSoldiersPress(Action *action)
{
	if (action->getDetails()->button.button != SDL_BUTTON_LEFT
	&&  action->getDetails()->button.button != SDL_BUTTON_RIGHT) return;

	int selRow = _lstSoldiers->getSelectedRow();
	if (_btnToSoldier->getPressed())
	{ // So we are setting a layout to a soldier
		EquipmentLayout *layout = (_selectedLayout == 0) ? 0 : _save->getLayouts()->at(_selectedLayout-1);
		if (selRow == 0)
		{ // Newly recruited soldiers is clicked
			_save->setNewSoldierLayoutId((layout == 0) ? 0 : layout->getId());
			_lstSoldiers->setCellText(0, 1, (layout == 0) ? tr("STR_CUSTOM").c_str() : layout->getName().c_str()); // Refresh the soldier-table
		}
		else
		{ // A soldier is clicked
			BattleUnit *soldierBU = _soldiers.at(selRow-1);
			soldierBU->getGeoscapeSoldier()->setEquipmentLayout(layout);
			if (layout == 0 || soldierBU->equipByLayout(_game, false))
			{ // Success
				_lstSoldiers->setCellText(selRow, 1, (layout == 0) ? tr("STR_CUSTOM").c_str() : layout->getName().c_str()); // Refresh the soldier-table
			}
			else
			{ // So there were not enough equipment for the layout
				_lstSoldiers->setCellText(selRow, 1, tr("STR_CUSTOM").c_str()); // Refresh the soldier-table
				_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_EQUIPMENT_FOR_LAYOUT"));
			}
		}
	}
	else
	{ // So we are saving the current layout of a soldier to a named layout
		if (_selectedLayout == 0)
		{ // Save to custom layout ?!?
			_warning->showMessage(_game->getLanguage()->getString("STR_CANNOT_SAVE_TO_CUSTOM_LAYOUT"));
		}
		else
		{ // Save to a named layout
			EquipmentLayout *layout = _save->getLayouts()->at(_selectedLayout-1);
			EquipmentLayout *soldierLayout;
			if (selRow == 0) layout->copyLayoutItems((_save->getNewSoldierLayoutId() == 0) ? 0 : _save->getLayout(_save->getNewSoldierLayoutId()));
			else
			{ // So an actual soldier is clicked
				layout->eraseItems();
				std::vector<EquipmentLayoutItem*> *items = layout->getItems();
				BattleUnit *soldier = _soldiers.at(selRow-1);
				for (std::vector<BattleItem*>::iterator j = soldier->getInventory()->begin(); j != soldier->getInventory()->end(); ++j)
				{
					std::string ammo;
					if ((*j)->needsAmmo() && (*j)->getAmmoItem() != 0) ammo = (*j)->getAmmoItem()->getRules()->getType();
					else ammo = "NONE";
					items->push_back(new EquipmentLayoutItem(
						(*j)->getRules()->getType(),
						(*j)->getSlot()->getId(),
						(*j)->getSlotX(),
						(*j)->getSlotY(),
						ammo,
						(*j)->getFuseTimer()
					));
				}
			}

			// Show a message of the success
			std::wstring soldierName;
			if (selRow == 0) soldierName = tr("STR_NEWLY_RECRUITED_SOLDIERS");
			else soldierName = _soldiers.at(selRow-1)->getGeoscapeSoldier()->getName();
			_warning->showMessage(_game->getLanguage()->getString("STR_LAYOUT_SAVED").arg(soldierName).arg(layout->getName()));
			
			// Ok, now refresh every soldier who has this layout
			for (int i = 0; i < _soldiers.size(); ++i)
			{
				BattleUnit *soldierBU = _soldiers.at(i);
				if (layout == soldierBU->getGeoscapeSoldier()->getEquipmentLayout())
				{
					if (!soldierBU->equipByLayout(_game, false))
					{ // So there were not enough equipment for the changed layout
						_lstSoldiers->setCellText(i+1, 1, tr("STR_CUSTOM").c_str()); // Refresh the soldier-table
						_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_EQUIPMENT_FOR_LAYOUT"));
					}
				}
			}
		}
	}
}

/**
 * Occurs when the player presses keyboard button (typing the layout name).
 * @param action Pointer to an action.
 */
void LayoutManagerState::edtLayoutKeyPress(Action *action)
{
	updateEditButtons();
}

/**
 * Occurs when the player clicks on the Create button.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnCreateClick(Action *action)
{
	int NextButton = _save->getLayouts()->size()+1;
	EquipmentLayout *layout = new EquipmentLayout(_edtLayout->getText(), _save->getId("LAYOUTS"));
	_save->getLayouts()->insert(_save->getLayouts()->begin()+_selectedLayout, layout); // Insert after the currently selected
	if (_save->getLayouts()->size()-1 < 5) addLayoutButton(L"", NextButton); // So we need a new button for it
	++_selectedLayout; // Select the newly created layout
	if (_selectedLayout - _btnLayoutsStartIndex > 5) _btnLayoutsStartIndex = _selectedLayout-5; // And scroll down if it's out of the window
	if (_selectedLayout - _btnLayoutsStartIndex < 0) _btnLayoutsStartIndex = _selectedLayout; // ...or up
	updateLayoutsButtons();
	updateEditButtons();
}

/**
 * Occurs when the player clicks on the Rename button.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnRenameClick(Action *action)
{
	EquipmentLayout *layout = _save->getLayouts()->at(_selectedLayout-1);
	int buttonIndex = _selectedLayout - _btnLayoutsStartIndex;
	layout->setName(_edtLayout->getText());
	if (buttonIndex >= 0 && buttonIndex < 6) _btnLayouts.at(buttonIndex)->setText(_edtLayout->getText());

	// Refresh the soldier-table
	for (int i = 0; i < _soldiers.size(); ++i)
	{
		Soldier *soldier = _soldiers.at(i)->getGeoscapeSoldier();
		if (layout == soldier->getEquipmentLayout())
		{
			_lstSoldiers->setCellText(i+1, 1, _edtLayout->getText().c_str()); // Refresh the soldier-table
		}
	}
	// And don't forget the newly recruited soldier
	EquipmentLayout *newSoldierLayout = (_save->getNewSoldierLayoutId() == 0) ? 0 : _save->getLayout(_save->getNewSoldierLayoutId());
	if (layout == newSoldierLayout)
	{
		_lstSoldiers->setCellText(0, 1, _edtLayout->getText().c_str()); // Refresh the soldier-table
	}
}

/**
 * Occurs when the player clicks on the Delete button.
 * @param action Pointer to an action.
 */
void LayoutManagerState::btnDeleteClick(Action *action)
{
	EquipmentLayout *layout = _save->getLayouts()->at(_selectedLayout-1);
	if (layout->getId() == 0) return; // Theoretically this should never happen

	// Ok, first of all, copy this layout to custom owned layouts for every soldier who has this layout
	for (int i = 0; i < _soldiers.size(); ++i)
	{
		Soldier *soldier = _soldiers.at(i)->getGeoscapeSoldier();
		if (layout == soldier->getEquipmentLayout())
		{
			soldier->setEquipmentLayout(new EquipmentLayout(layout));
			_lstSoldiers->setCellText(i+1, 1, tr("STR_CUSTOM").c_str()); // Refresh the soldier-table
		}
	}
	// And don't forget the newly recruited soldier
	EquipmentLayout *newSoldierLayout = (_save->getNewSoldierLayoutId() == 0) ? 0 : _save->getLayout(_save->getNewSoldierLayoutId());
	if (layout == newSoldierLayout)
	{
		_save->setNewSoldierLayoutId(0);
		_lstSoldiers->setCellText(0, 1, tr("STR_CUSTOM").c_str()); // Refresh the soldier-table
	}

	// Now erase the layout
	delete layout;
	_save->getLayouts()->erase(_save->getLayouts()->begin()+(_selectedLayout-1));
	if (_selectedLayout > _save->getLayouts()->size()) --_selectedLayout; // If we erased the last layout
	if (_btnLayoutsStartIndex+5 > _save->getLayouts()->size()) --_btnLayoutsStartIndex; // If the window is out of limits
	if (_btnLayoutsStartIndex < 0) _btnLayoutsStartIndex = 0; // If we overshot with the previous
	if (_selectedLayout - _btnLayoutsStartIndex > 5) _btnLayoutsStartIndex = _selectedLayout-5; // And scroll down if the newly selected is out of the window
	if (_selectedLayout - _btnLayoutsStartIndex < 0) _btnLayoutsStartIndex = _selectedLayout; // ...or up
	if (_save->getLayouts()->size() < 5)
	{ // So we need one button less
		std::vector<ToggleTextButton*>::iterator button = _btnLayouts.begin() + (_btnLayouts.size()-1);
		for (std::vector<Surface*>::iterator i = _surfaces.begin(); i < _surfaces.end(); ++i)
		{
			if (*i == *button)
			{
				delete *i;
				_surfaces.erase(i);
				break;
			}
		}
		_btnLayouts.erase(button);
	}
	updateLayoutsButtons();
	updateEditButtons();
}

}