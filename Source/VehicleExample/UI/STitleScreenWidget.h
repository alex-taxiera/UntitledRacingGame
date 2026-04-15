// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class URacingGameInstance;

/**
 * STitleScreenWidget
 *
 * Pure Slate title screen for Untitled Racing Game.
 *
 * Layout (full-screen, black background):
 *
 *   ???????????????????????????????
 *   ?                             ?
 *   ?     UNTITLED RACING GAME    ?   ? large title
 *   ?                             ?
 *   ?        [ NEW GAME ]         ?
 *   ?        [ CONTINUE ]         ?   ? greyed + disabled when no save
 *   ?        [ DELETE SAVE ]      ?   ? hidden when no save
 *   ?        [ EXIT ]             ?
 *   ?                             ?
 *   ???????????????????????????????
 *
 * When "DELETE SAVE" is clicked a confirmation overlay slides over the buttons:
 *
 *   ???????????????????????????????
 *   ?  Delete all save data?      ?
 *   ?  This cannot be undone.     ?
 *   ?                             ?
 *   ?   [ CONFIRM ]  [ CANCEL ]   ?
 *   ???????????????????????????????
 */
class VEHICLEEXAMPLE_API STitleScreenWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(STitleScreenWidget)
        : _GameInstance(nullptr)
    {}
        SLATE_ARGUMENT(URacingGameInstance*, GameInstance)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    TWeakObjectPtr<URacingGameInstance> GameInstance;

    /** True while the delete-confirmation overlay is visible. */
    bool bConfirmDeleteVisible = false;

    // -----------------------------------------------------------------------
    // Button callbacks
    // -----------------------------------------------------------------------

    FReply OnNewGameClicked();
    FReply OnContinueClicked();
    FReply OnDeleteSaveClicked();
    FReply OnConfirmDeleteClicked();
    FReply OnCancelDeleteClicked();
    FReply OnExitClicked();

    // -----------------------------------------------------------------------
    // Attribute bindings
    // -----------------------------------------------------------------------

    /** Continue button enabled only when a save file exists. */
    bool IsContinueEnabled() const;

    /** "Delete Save" row hidden when no save exists. */
    EVisibility GetDeleteSaveVisibility() const;

    /** Confirmation overlay visibility. */
    EVisibility GetConfirmOverlayVisibility() const;

    /** Main button column visibility (hidden while confirm overlay is shown). */
    EVisibility GetMainButtonsVisibility() const;

    // -----------------------------------------------------------------------
    // Style helpers
    // -----------------------------------------------------------------------

    /** Returns a standard menu button style appropriate for the state. */
    static const FButtonStyle& GetMenuButtonStyle();
};
