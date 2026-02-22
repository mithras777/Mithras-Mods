Scriptname taro_kickmod_MEffect_self_effect extends activemagiceffect  

Quest Property Taro_kickmod_quest  Auto  

taro_kickmod_MCM Property Kick_MCM_Settings Auto

Idle Property taro_kickmod_kick Auto
Idle Property taro_kickmod_kick_high Auto
Idle Property taro_kickmod_kick_medium Auto

Idle Property taro_kickmod_kick_TPP Auto
Idle Property taro_kickmod_kick_high_TPP Auto
Idle Property taro_kickmod_kick_medium_TPP Auto

Idle Property resetAnim Auto
Idle Property resetAnimMagic Auto

Sound Property SoundSwing  Auto  

bool animationPlayed = false

Event OnEffectStart(Actor akTarget, Actor akCaster)
    ; Right now we don't play an animation if player is moving forward as the idle stops the player midair

    ; check if player is in 1st person
    ; Debug.Notification("1st person: " + Game.GetPlayer().GetAnimationVariableInt("i1stPerson"))



    if (Kick_MCM_Settings.playKickAnimation) && !(Game.GetPlayer().GetAnimationVariableFloat("Speed") > 10)
        
        animationPlayed = true

        float viewAngle = Game.GetPlayer().GetAngleX()

        If (Game.GetPlayer().GetAnimationVariableInt("i1stPerson") == 1)
            If (viewAngle <= -10)
            ; debug.Notification("Player is looking up")
            Game.GetPlayer().playIdle(taro_kickmod_kick_high)
            ; player is looking up, play high kick animation
            elseIf (viewAngle >= 25)
                ; debug.Notification("Player is looking down")
                Game.GetPlayer().playIdle(taro_kickmod_kick)
                ; player is looking down, play low kick animation
            ElseIf (true)
                ; debug.Notification("Player is looking straight")
                Game.GetPlayer().playIdle(taro_kickmod_kick_medium)
                ; default case play medium kick animation
            EndIf
        else
            If (viewAngle <= -10)
            ; debug.Notification("Player is looking up")
            Game.GetPlayer().playIdle(taro_kickmod_kick_high_TPP)
            ; player is looking up, play high kick animation
            elseIf (viewAngle >= 25)
                ; debug.Notification("Player is looking down")
                Game.GetPlayer().playIdle(taro_kickmod_kick_TPP)
                ; player is looking down, play low kick animation
            ElseIf (true)
                ; debug.Notification("Player is looking straight")
                Game.GetPlayer().playIdle(taro_kickmod_kick_medium_TPP)
                ; default case play medium kick animation
            EndIf
        EndIf
        ; Debug.Notification("View angle: " + viewAngle)
        

        ; Game.GetPlayer().playIdle(taro_kickmod_kick)
    endif
    SoundSwing.play(Game.GetPlayer())
    Game.ShakeCamera(Game.GetPlayer(), 1, Kick_MCM_Settings.kickScreenshakeDuration)
    Game.GetPlayer().DamageActorValue("Stamina", Kick_MCM_Settings.kickStaminaCost)
    
    ; Debug.Notification("Trying to start kick reset")
    if  (Game.GetPlayer().IsWeaponDrawn())
        If !(Kick_MCM_Settings.applyAnimationFixInFirstPerson) && (Game.GetPlayer().GetAnimationVariableInt("i1stPerson") == 1)
        Else
            If (animationPlayed)
                StartKickReset()
            EndIf
        EndIf
    EndIf
    
EndEvent

Function StartKickReset()
    ; Debug.Notification("Start Kick Reset")
    ; Start a small async wait before allowing kick again
    Utility.Wait(0.60)
    Spell rightSpell = Game.GetPlayer().GetEquippedSpell(0)
    if rightSpell != None
        Game.GetPlayer().playIdle(resetAnimMagic)
    else
        Game.GetPlayer().playIdle(resetAnim)
    endif
    
EndFunction