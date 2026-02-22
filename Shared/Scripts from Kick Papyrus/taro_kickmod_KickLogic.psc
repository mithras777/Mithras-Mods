Scriptname taro_kickmod_KickLogic extends Quest

taro_kickmod_MCM Property Kick_MCM_Settings Auto

SPELL Property TimeSlowSpell  Auto  

SPELL Property hostileSpell  Auto  

Sound Property HitSoundMedium  Auto  

Sound Property HitSoundHeavy  Auto  

Sound Property SoundSwing  Auto  

SPELL Property spell_lightStagger  Auto  

SPELL Property spell_heavyStagger  Auto  

SPELL Property KickTriggerSpell  Auto  

SPELL Property JumpKickTriggerSpell  Auto

SPELL Property RunningJumpKickTriggerSpell  Auto

SPELL Property SelfEffectSpell  Auto

float playerStamina = 0.0

bool canKick = true



Event OnKeyDown(int keyCode)

  ; Debug.MessageBox("Key Pressed")
  if UI.IsTextInputEnabled()
    return
  endif
  if keyCode != Kick_MCM_Settings.KickKeycode
    return
  endif
  playerStamina = Game.GetPlayer().GetAV("Stamina") as int
  if playerStamina < Kick_MCM_Settings.kickStaminaCost
    return
  endif
  if !canKick
    ; Debug.Notification("Can't kick yet.")
    return
  endif

  canKick = false

  
  

  SelfEffectSpell.Cast(Game.GetPlayer(), Game.GetPlayer())
  ; Debug.Notification("Trying to cast a spell")

  if Game.GetPlayer().GetAnimationVariableBool("bInJumpState")
    JumpKickTriggerSpell.Cast(Game.GetPlayer(), None)
  ElseIf (Game.GetPlayer().GetAnimationVariableFloat("Speed") > 10)
    RunningJumpKickTriggerSpell.Cast(Game.GetPlayer(), None)
  else
    KickTriggerSpell.Cast(Game.GetPlayer(), None)
  endif
  

  StartKickCooldown()

EndEvent



Function StartKickCooldown()
    ; Debug.Notification("Start Kick Cooldown")
    ; Start a small async wait before allowing kick again
    Utility.Wait(Kick_MCM_Settings.kickCooldown)
    ; Debug.Notification("Kick Cooldown Ended")
    canKick = true
EndFunction
