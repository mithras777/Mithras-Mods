Scriptname taro_kickmod_MCM extends SKI_ConfigBase

; Keymap Value
int property KickKeycode = 0x21 auto ; F

; State
float property baseKickForce = 10.0 auto
float property baseKickDamage = 1.0 auto
float property additionalDamagePerStamina = 0.01 auto
float property kickStaminaCost = 20.0 auto
float property armorRaitingMultiplier = 0.24 auto
bool  property slowTimeOnKick = false auto
bool  property playKickAnimation = true auto
bool  property applyAnimationFixInFirstPerson = true auto
float property kickScreenshakeDuration = 0.3 auto
float property additionalVelocityForce = 1.0 auto
float property jumpKickBonus = 10.0 auto
float property backKickBonus = 15.0 auto
float property kickCooldown = 2.0 auto

; float property kickRange = 200.0 auto


float property baseKickStaminaDamage = 5.0 auto
float property additionalStaminaDamagePerStamina = 0.25 auto


float property knockdownHealthThreshold = 50.0 auto
float property knockdownStaminaThreshold = 25.0 auto

bool property canKick = true auto


; MCM Options
int oid_keyMapOption
int baseKickForceOID_S ; _S for slider
int kickCooldownOID_S ; _S for slider
int baseKickDamageOID_S ; _S for slider
int additionalDamagePerStaminaOID_S ; _S for slider
int armorRaitingMultiplierOID_S ; _S for slider
int kickStaminaCostOID_S ; _S for slider
int additionalVelocityForceOID_S ; _S for slider
int jumpKickBonusOID_S ; _S for slider
int backKickBonusOID_S ; _S for slider
int baseKickStaminaDamageOID_S ; _S for slider
int additionalStaminaDamagePerStaminaOID_S ; _S for slider
int knockdownHealthThresholdOID_S ; _S for slider
int knockdownStaminaThresholdOID_S ; _S for slider

int kickRangeOID_S ; _S for slider

; Unused Keys

bool RequireCtrlKey = false
bool RequireAltKey = false
bool RequireShiftKey = false

; unused Options
int kickScreenshakePowerOID_S ; _S for slider
int kickScreenshakeDurationOID_S ; _S for slider

;unused States
float kickScreenshakePower = 2.0

Event OnConfigInit()

  Modname = "Kick Mod"
  Pages = new string[1]
  Pages[0] = "Settings"
  RegisterForKey(KickKeycode)
EndEvent


; set up the MCM page
Event OnPageReset(string page)
  AddKeyMapOptionST("KickKeybind", "Kick Key", KickKeycode)
  AddToggleOptionST("playKickAnimationToggle", "Play Kick Animation", playKickAnimation)
  AddToggleOptionST("applyAnimationFixInFirstPersonToggle", "Apply Animation Fix in First Person", applyAnimationFixInFirstPerson)
  AddToggleOptionST("SlowTimeToggle", "Slow Time on Kick", slowTimeOnKick)
  ; kickRangeOID_S = AddSliderOption("Kick Range", kickRange, "{0}")
  kickCooldownOID_S = AddSliderOption("Kick Cooldown", kickCooldown, "{2}s")
  baseKickForceOID_S = AddSliderOption("Base Kick Force", baseKickForce, "{0}")
  baseKickDamageOID_S = AddSliderOption("Base Kick Damage", baseKickDamage, "{0}")
  additionalDamagePerStaminaOID_S = AddSliderOption("Extra Damage per Stamina", additionalDamagePerStamina, "{2}")
  
  baseKickStaminaDamageOID_S = AddSliderOption("Base Kick Stamina Damage", baseKickStaminaDamage, "{0}")
  additionalStaminaDamagePerStaminaOID_S = AddSliderOption("Extra Stamina Damage per Stamina", additionalStaminaDamagePerStamina, "{2}")

  knockdownHealthThresholdOID_S = AddSliderOption("Knockdown Health Threshold", knockdownHealthThreshold, "{0}%")
  knockdownStaminaThresholdOID_S = AddSliderOption("Knockdown Stamina Threshold", knockdownStaminaThreshold, "{0}%")

  jumpKickBonusOID_S = AddSliderOption("Jump Kick Bonus", jumpKickBonus, "{0}%")
  backKickBonusOID_S = AddSliderOption("Back Kick Bonus", backKickBonus, "{0}%")
  armorRaitingMultiplierOID_S = AddSliderOption("Armor Raiting Multiplier", armorRaitingMultiplier, "{2}")
  kickStaminaCostOID_S = AddSliderOption("Kick Stamina Cost", kickStaminaCost, "{0}")

  additionalVelocityForceOID_S = AddSliderOption("Additional force from player velocity", additionalVelocityForce, "{2}")

  
  AddKeyMapOptionST("KickKeybind", "Kick Key", KickKeycode)

  ; unused Options
  
  ; AddToggleOptionST("CtrlKeyToggle", "Ctrl", RequireCtrlKey)
  ; AddToggleOptionST("AltKeyToggle", "Alt", RequireAltKey)
  ; AddToggleOptionST("ShiftKeyToggle", "Shift", RequireShiftKey)

  ; additionalVelocityForceOID_S = AddSliderOption("Additional force from player velocity", additionalVelocityForce, "{0}")
  ; kickScreenshakePowerOID_S = AddSliderOption("Screenshake Power", kickScreenshakePower, "{0.0}")
  ; kickScreenshakeDurationOID_S = AddSliderOption("Screenshake Duration", kickScreenshakeDuration)

EndEvent

; slider value selection
event OnOptionSliderOpen(int option)
	if (option == baseKickForceOID_S)
		SetSliderDialogStartValue(baseKickForce)
		SetSliderDialogDefaultValue(10.0)
		SetSliderDialogRange(0.0, 100.0)
		SetSliderDialogInterval(1)

    elseif (option == kickStaminaCostOID_S)
		SetSliderDialogStartValue(kickStaminaCost)
		SetSliderDialogDefaultValue(20)
		SetSliderDialogRange(0.00, 100.00)    
		SetSliderDialogInterval(1)

    elseif (option == baseKickDamageOID_S)
		SetSliderDialogStartValue(baseKickDamage)
		SetSliderDialogDefaultValue(1)
		SetSliderDialogRange(0.00, 200.00)    
		SetSliderDialogInterval(1)

    elseif (option == additionalDamagePerStaminaOID_S)
		SetSliderDialogStartValue(additionalDamagePerStamina)
		SetSliderDialogDefaultValue(0.01) 
		SetSliderDialogRange(0.00, 1.00)    
		SetSliderDialogInterval(0.01)

    elseif (option == armorRaitingMultiplierOID_S)
		SetSliderDialogStartValue(armorRaitingMultiplier)
		SetSliderDialogDefaultValue(0.24) 
		SetSliderDialogRange(0.00, 1.00)    
		SetSliderDialogInterval(0.01)

    elseif (option == additionalVelocityForceOID_S)
		SetSliderDialogStartValue(additionalVelocityForce)
		SetSliderDialogDefaultValue(1.00)
		SetSliderDialogRange(0.00, 10.00)    
		SetSliderDialogInterval(0.1)

    elseif (option == jumpKickBonusOID_S)
		SetSliderDialogStartValue(jumpKickBonus)
		SetSliderDialogDefaultValue(10.00)
		SetSliderDialogRange(0.00, 100.00)    
		SetSliderDialogInterval(1)

    elseif (option == backKickBonusOID_S)
		SetSliderDialogStartValue(backKickBonus)
		SetSliderDialogDefaultValue(15.00)
		SetSliderDialogRange(0.00, 100.00)    
		SetSliderDialogInterval(1)

    elseIf (option == kickCooldownOID_S)
		SetSliderDialogStartValue(kickCooldown)
		SetSliderDialogDefaultValue(2.00)
		SetSliderDialogRange(1.00, 10.00)    
		SetSliderDialogInterval(0.1)

    
    elseif (option == baseKickStaminaDamageOID_S)
		SetSliderDialogStartValue(baseKickStaminaDamage)
		SetSliderDialogDefaultValue(5)
		SetSliderDialogRange(0.00, 200.00)    
		SetSliderDialogInterval(1)

    elseif (option == additionalStaminaDamagePerStaminaOID_S)
		SetSliderDialogStartValue(additionalStaminaDamagePerStamina)
		SetSliderDialogDefaultValue(0.25) 
		SetSliderDialogRange(0.00, 1.00)    
		SetSliderDialogInterval(0.01)

    elseif (option == knockdownHealthThresholdOID_S)
		SetSliderDialogStartValue(knockdownHealthThreshold)
		SetSliderDialogDefaultValue(50)
		SetSliderDialogRange(0.00, 100.00)    
		SetSliderDialogInterval(1)

    elseif (option == knockdownStaminaThresholdOID_S)
		SetSliderDialogStartValue(knockdownStaminaThreshold)
		SetSliderDialogDefaultValue(25)
		SetSliderDialogRange(0.00, 100.00)    
		SetSliderDialogInterval(1)

    ; elseif (option == kickRangeOID_S)
		; SetSliderDialogStartValue(kickRange)
		; SetSliderDialogDefaultValue(200)
		; SetSliderDialogRange(1.00, 200.00)    
		; SetSliderDialogInterval(1)

    
    ; unused Options

    elseif (option == kickScreenshakePowerOID_S)
		SetSliderDialogStartValue(kickScreenshakePower)
		SetSliderDialogDefaultValue(2.5)
		SetSliderDialogRange(0.00, 1)    
		SetSliderDialogInterval(0.5)

    elseif (option == kickScreenshakeDurationOID_S)
		SetSliderDialogStartValue(kickScreenshakeDuration)
		SetSliderDialogDefaultValue(1)
		SetSliderDialogRange(0.00, 1.00)    
		SetSliderDialogInterval(0.05)
	endIf
endEvent

; slider value acceptance
event OnOptionSliderAccept(int option, float value)
	if (option == baseKickForceOID_S)
		baseKickForce = value
		SetSliderOptionValue(baseKickForceOID_S, baseKickForce, "{0}")

    elseif (option == kickStaminaCostOID_S)
		kickStaminaCost = value    
		SetSliderOptionValue(kickStaminaCostOID_S, kickStaminaCost, "{0}")

    elseif (option == baseKickDamageOID_S)
		baseKickDamage = value    
		SetSliderOptionValue(baseKickDamageOID_S, baseKickDamage, "{0}")

    elseif (option == additionalDamagePerStaminaOID_S)
		additionalDamagePerStamina = value    
		SetSliderOptionValue(additionalDamagePerStaminaOID_S, additionalDamagePerStamina, "{2}")

    elseif (option == armorRaitingMultiplierOID_S)
		armorRaitingMultiplier = value    
		SetSliderOptionValue(armorRaitingMultiplierOID_S, armorRaitingMultiplier, "{2}")
    
    elseif (option == additionalVelocityForceOID_S)
		additionalVelocityForce = value    
		SetSliderOptionValue(additionalVelocityForceOID_S, additionalVelocityForce, "{2}")

    elseIf (option == jumpKickBonusOID_S)
		jumpKickBonus = value    
		SetSliderOptionValue(jumpKickBonusOID_S, jumpKickBonus, "{0}%")

    elseif (option == backKickBonusOID_S)
		backKickBonus = value    
		SetSliderOptionValue(backKickBonusOID_S, backKickBonus, "{0}%")

    elseif (option == kickCooldownOID_S)
		kickCooldown = value
		SetSliderOptionValue(kickCooldownOID_S, kickCooldown, "{2}s")

    
    elseif (option == baseKickStaminaDamageOID_S)
		baseKickStaminaDamage = value    
		SetSliderOptionValue(baseKickStaminaDamageOID_S, baseKickStaminaDamage, "{0}")

    elseif (option == additionalStaminaDamagePerStaminaOID_S)
		additionalStaminaDamagePerStamina = value    
		SetSliderOptionValue(additionalStaminaDamagePerStaminaOID_S, additionalStaminaDamagePerStamina, "{2}")


    ElseIf (option == knockdownHealthThresholdOID_S)
      knockdownHealthThreshold = value    
      SetSliderOptionValue(knockdownHealthThresholdOID_S, knockdownHealthThreshold, "{0}%")
    
    ElseIf (option == knockdownStaminaThresholdOID_S)
      knockdownStaminaThreshold = value    
      SetSliderOptionValue(knockdownStaminaThresholdOID_S, knockdownStaminaThreshold, "{0}%")
    ; unused Options

    elseif (option == kickScreenshakePowerOID_S)
		kickScreenshakePower = value    
		SetSliderOptionValue(kickScreenshakePowerOID_S, kickScreenshakePower, "{0}")

    elseif (option == kickScreenshakeDurationOID_S)
		kickScreenshakeDuration = value    
		SetSliderOptionValue(kickScreenshakeDurationOID_S, kickScreenshakeDuration, "{0}")

    ; elseif (option == kickRangeOID_S)
		; kickRange = value    
		; SetSliderOptionValue(kickRangeOID_S, kickRange, "{0} cm")
	endIf
endEvent

; keyboard shortcut for the kick
State KickKeybind

  Event OnKeyMapChangeST(int keyCode, string conflictControl, string conflictName)
    UnregisterForKey(KickKeycode)
    KickKeycode = keyCode
    RegisterForKey(KickKeycode)
    SetKeyMapOptionValueST(KickKeycode)
  EndEvent
  Event OnHighlightST()
    SetInfoText("Press the key you want to use for kicking.")
  EndEvent
EndState



; Toggles for the modifier keys

State SlowTimeToggle
  Event OnSelectST()
    slowTimeOnKick = !slowTimeOnKick
    SetToggleOptionValueST(slowTimeOnKick)
  EndEvent
  Event OnHighlightST()
    SetInfoText("Slow Time on Kick")
  EndEvent
EndState

State applyAnimationFixInFirstPersonToggle
  Event OnSelectST()
    applyAnimationFixInFirstPerson = !applyAnimationFixInFirstPerson
    SetToggleOptionValueST(applyAnimationFixInFirstPerson)
  EndEvent
  Event OnHighlightST()
    SetInfoText("Apply Animation Fix in First Person. This will play draw weapon animation in first person. Due to engine limitations kicking locks your character into idle stance. Draw weapon animation is required to break out of the idle stance. It's a bit of a less of an issue in first person with is why this toggle has been added. Use at your own risk.")
  EndEvent
EndState

State playKickAnimationToggle
  Event OnSelectST()
    playKickAnimation = !playKickAnimation
    SetToggleOptionValueST(playKickAnimation)
  EndEvent
  Event OnHighlightST()
    SetInfoText("Play Kick Animation. Only when standing still, as it causes your character to stop moving if you are running or jumping. Which breaks speed calculations and feels bad. However not much can be done to fix that.")
  EndEvent
EndState

Event OnOptionHighlight(int option)
    if option == baseKickForceOID_S
        SetInfoText("Base Kick Force: how much physical force is applied to enemies when kicked. Higher values send enemies flying farther.")
    elseif option == kickStaminaCostOID_S
        SetInfoText("Kick Stamina Cost: amount of player's stamina consumed per kick.")
    elseif option == kickCooldownOID_S
        SetInfoText("Kick Cooldown: how long the player has to wait before they can kick again.")
    elseif option == baseKickDamageOID_S
        SetInfoText("Base Kick Damage: how much damage is applied to enemies when kicked. Higher values deal more damage.")
    elseif option == additionalDamagePerStaminaOID_S
        SetInfoText("Extra Damage per Stamina: extra damage per each current stamina point. Higher values deal more damage.")
    elseif option == baseKickStaminaDamageOID_S
        SetInfoText("Base Kick Stamina Damage: base amount of stamina damage applied to enemies when kicked. Higher values deal more damage.")
    elseif option == additionalStaminaDamagePerStaminaOID_S
        SetInfoText("Extra Stamina Damage per Stamina: extra stamina damage per each current stamina point. Higher values deal more damage.")
    elseif option == knockdownHealthThresholdOID_S
        SetInfoText("Knockdown Health Threshold: if the target's current precentage of their total health is under this threshold, they can be knocked down. Otherwise they will stagger instead. This is a percentage of their total health.")
    elseif option == knockdownStaminaThresholdOID_S
        SetInfoText("Knockdown Stamina Threshold: if the target's current precentage of their total stamina is under this threshold, they can be knocked down. Otherwise they will stagger instead. This is a percentage of their total stamina.")
    elseif option == jumpKickBonusOID_S
        SetInfoText("Jump Kick Bonus: bonus percentage of the base and additional kick force and damage applied when the player kicks while in the air.")
    elseif option == backKickBonusOID_S
        SetInfoText("Back Kick Bonus: bonus percentage of the base and additional kick force and damage applied when the player kicks the enemy from behind.")
    elseif option == armorRaitingMultiplierOID_S
        SetInfoText("Armor Raiting Multiplie: each point of armor reduces the damage and kick force taken by this percentage. Higher values reduce the damage taken more.")
    elseif option == additionalVelocityForceOID_S
        SetInfoText("Additional force from player velocity: each point of speed increases kick this much.")
    elseif option == kickRangeOID_S
        SetInfoText("Kick range in centimeters. This is the distance the player can kick an enemy from.")
    else
        SetInfoText("") ; clear for other options (optional)
    endif
EndEvent

State CtrlKeyToggle
  Event OnSelectST()
    RequireCtrlKey = !RequireCtrlKey
    SetToggleOptionValueST(RequireCtrlKey)
  EndEvent
  Event OnHighlightST()
    SetInfoText("Require Ctrl Key")
  EndEvent
EndState

State AltKeyToggle
  Event OnSelectST()
    RequireAltKey = !RequireAltKey
    SetToggleOptionValueST(RequireAltKey)
  EndEvent
  Event OnHighlightST()
    SetInfoText("Require Alt Key")
  EndEvent
EndState

State ShiftKeyToggle
  Event OnSelectST()
    RequireShiftKey = !RequireShiftKey
    SetToggleOptionValueST(RequireShiftKey)
  EndEvent
  Event OnHighlightST()
    SetInfoText("Require Shift Key")
  EndEvent
EndState
