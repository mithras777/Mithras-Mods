Scriptname taro_kickmod_MEffect_trigger extends activemagiceffect  

Quest Property Taro_kickmod_quest  Auto  

taro_kickmod_MCM Property Kick_MCM_Settings Auto

SPELL Property TimeSlowSpell  Auto  

SPELL Property hostileSpell  Auto  

Sound Property HitSoundMedium  Auto  

Sound Property HitSoundHeavy  Auto  

Sound Property SoundSwing  Auto  

SPELL Property spell_lightStagger  Auto  

SPELL Property spell_heavyStagger  Auto 

float playerStamina = 0.0


Event OnInit()
  ; Debug.Notification("Kick spell is cast")


EndEvent

Event OnEffectStart(actor Target, actor Caster)

  

  ; we need to check the range between the player and the enemy. If it's larger than the kick range, we will not kick and return

  ; Debug.Notification("Target distance: " + Target.GetDistance(Caster))
  ; if (Target.GetDistance(Caster) > Kick_MCM_Settings.kickRange)
  ;   Debug.Notification("Target is too far away")
  ;   return
  ; endif



  float relativeDirection = math.abs(Target.GetHeadingAngle(Caster))
    bool playerIsBehind = false
    bool playerIsBeside = false

    If relativeDirection >= 140
      playerIsBehind = true
    elseif relativeDirection >= 70
      playerIsBeside = true
    endif

    bool targetIsBlocking = Target.GetAnimationVariableBool("Isblocking")
    bool targetIsAttacking = Target.GetAnimationVariableBool("IsAttacking")
    bool targetIsRecoiling = Target.GetAnimationVariableBool("IsRecoiling")
    bool targetIsStaggering = Target.GetAnimationVariableBool("IsStaggering")
    bool targetIsBleedingOut = Target.GetAnimationVariableBool("IsBleedingOut")
    bool targetKnowsAboutPlayer = Caster.IsDetectedBy(Target)
    bool targetIsInCombatMode = Target.IsInCombat()

    ; we need to check if taget is getting up, because the stagger does not work on that, so we will just knock them down, but we need to know if they are actually getting up at the moment of the kick

    bool targetIsGetingUp = false

    If Target.GetAnimationVariableInt("iState") == 0
      targetIsGetingUp = true
    EndIf




    ; big if stack that checks the state of the target and decides what to do

    ; 0 - no effect
    ; 1 - light stagger
    ; 2 - heavy stagger
    ; 3 - knockdown
    int KickEffectToApply = 1

    if targetIsBlocking
      ; debug.Notification("Target is Blocking")
      if playerIsBehind
        KickEffectToApply = 2
      ElseIf playerIsBeside
        KickEffectToApply = 1
      ElseIf (true)
        KickEffectToApply = 1
      Endif
  
    elseif targetIsRecoiling
      ; debug.Notification("Target is Recoiling")
      if playerIsBehind
        KickEffectToApply = 3
      ElseIf playerIsBeside
        KickEffectToApply = 3
      ElseIf (true)
        KickEffectToApply = 2
      Endif
  
    elseif targetIsAttacking
      ; debug.Notification("Target is attacking")
      if playerIsBehind
        KickEffectToApply = 3
      ElseIf playerIsBeside
        KickEffectToApply = 2
      ElseIf (true)
        KickEffectToApply = 1
      Endif
    
    elseif targetIsStaggering
      ; debug.Notification("Target is Staggered")
      if playerIsBehind
        KickEffectToApply = 3
      ElseIf playerIsBeside
        KickEffectToApply = 3
      ElseIf (true)
        KickEffectToApply = 3
      Endif

    elseif targetIsBleedingOut
      KickEffectToApply = 3

    elseif targetIsGetingUp
      ; debug.Notification("Target is getting up")
      KickEffectToApply = 3

    ; default case, target is just standing there, menacingly
    elseif (true)
      ; debug.Notification("Default case")
      if playerIsBehind
        KickEffectToApply = 3
      ElseIf playerIsBeside
        KickEffectToApply = 2
      ElseIf (true)
        KickEffectToApply = 1
      Endif
    EndIf

    
    ; we also need to know if player is jumping as that brings the effect up a level
    bool playerIsJumping = Caster.GetAnimationVariableBool("bInJumpState")

    If (playerIsJumping) && (KickEffectToApply < 3)
      KickEffectToApply = KickEffectToApply + 1
    EndIf

    float targetStaminaPercentage = Target.GetActorValuePercentage("Stamina") *100
    float targetHealthPercentage = Target.GetActorValuePercentage("Health") *100



    ; Debug.Notification("Target Stamina %: " + targetStaminaPercentage + " Target Health %: " + targetHealthPercentage)

    ; if the kick is to konckodown, then we need to check if the target health and stamina are low enough to trigger the effect, if not we decrease the severity
    If (KickEffectToApply == 3)
      If !(!(targetIsInCombatMode) || !(targetKnowsAboutPlayer))
        If ((targetStaminaPercentage <= Kick_MCM_Settings.knockdownStaminaThreshold))
          ; Debug.Notification("Stamina % Thereshold: " + Kick_MCM_Settings.knockdownStaminaThreshold + " Target Stamina: " + targetStaminaPercentage)
        ElseIf ((targetHealthPercentage <= Kick_MCM_Settings.knockdownHealthThreshold))
          ; Debug.Notification("Health % Thereshold: " + Kick_MCM_Settings.knockdownHealthThreshold + " Target Health: " + targetHealthPercentage)
        ElseIf (true)
          ; Debug.Notification("Target is over the threshold, decreasing the effect")
          KickEffectToApply = 2
        EndIf
      EndIf
    EndIf

    ; if the target is not in combat mode or did not detect player we will always apply the knockdown effect, a sort of sneak attack

    if KickEffectToApply == 0
      ; Debug.Notification("No effect")
      NoEffectKick(Caster,Target)
      return
    elseif KickEffectToApply == 1
      ; Debug.Notification("Light Stagger")
      LightStaggerKick(Caster,Target)
    elseif KickEffectToApply == 2
      ; Debug.Notification("Heavy Stagger")
      HeavyStaggerKick(Caster,Target)
    elseif KickEffectToApply == 3
      ; Debug.Notification("Knockdown")
      KnockdownKick(Caster,Target)
    endif
  

EndEvent


Function NoEffectKick(Actor Caster, Actor Target)
  playerStamina = Caster.GetAV("Stamina") as int
  float playerVelocity = Caster.GetAnimationVariableFloat("Speed")
  ; Debug.Notification("Light stagger")

  hostileSpell.Cast(Caster, Target)
  HitSoundMedium.play(Caster)
  float DRMultiplier = CalculateDRMultiplier(Target)


  float staminaDamage = ((Kick_MCM_Settings.baseKickStaminaDamage + (playerStamina  * Kick_MCM_Settings.additionalStaminaDamagePerStamina)) * DRMultiplier) * 0.25


  staminaDamage = staminaDamage + ((playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)* 0.25)

  target.DamageActorValue("Stamina", staminaDamage)

EndFunction

Function LightStaggerKick(Actor Caster, Actor Target)
  playerStamina = Caster.GetAV("Stamina") as int
  float playerVelocity = Caster.GetAnimationVariableFloat("Speed")
  ; Debug.Notification("Light stagger")

  hostileSpell.Cast(Caster, target)
  spell_lightStagger.Cast(Caster, target)
  HitSoundMedium.play(Caster)
  float DRMultiplier = CalculateDRMultiplier(target)

  float damage = ((Kick_MCM_Settings.baseKickDamage + (playerStamina  * Kick_MCM_Settings.additionalDamagePerStamina)) * DRMultiplier) * 0.5

  float staminaDamage = ((Kick_MCM_Settings.baseKickStaminaDamage + (playerStamina  * Kick_MCM_Settings.additionalStaminaDamagePerStamina)) * DRMultiplier) * 0.5
  
  damage = damage + ((playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)* 0.5)


  staminaDamage = staminaDamage + ((playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)* 0.5)

  target.DamageActorValue("Stamina", staminaDamage)

  target.DamageActorValue("Health", damage)
EndFunction

; This function will be called in cases when we want enemy to stagger heavily
Function HeavyStaggerKick(Actor Caster, Actor Target)
  playerStamina = Caster.GetAV("Stamina") as int
  
  float playerVelocity = Caster.GetAnimationVariableFloat("Speed")
 

  hostileSpell.Cast(Caster, target)
  spell_heavyStagger.Cast(Caster, target)
  HitSoundHeavy.play(Caster)
  float DRMultiplier = CalculateDRMultiplier(target)

  float damage = ((Kick_MCM_Settings.baseKickDamage + (playerStamina  * Kick_MCM_Settings.additionalDamagePerStamina)) * DRMultiplier) * 0.75

  damage = damage + ((playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)* 0.75)

  float staminaDamage = ((Kick_MCM_Settings.baseKickStaminaDamage + (playerStamina  * Kick_MCM_Settings.additionalStaminaDamagePerStamina)) * DRMultiplier) * 0.75

  staminaDamage = staminaDamage + ((playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)* 0.75)

  target.DamageActorValue("Stamina", staminaDamage)

  target.DamageActorValue("Health", damage)

  ; recasting the stagger spell to trigger the heavy stagger animation, no way of doing it differently AFAIK
  Utility.wait(1)

  spell_heavyStagger.Cast(Caster, target)
  
  Utility.wait(0.4)
  spell_heavyStagger.Cast(Caster, target)

  Utility.wait(0.5)
  spell_heavyStagger.Cast(Caster, target)
EndFunction


; This functiion will be called when we want to knock down targed and send them flying
Function KnockdownKick(Actor Caster, Actor Target)
    playerStamina = Caster.GetAV("Stamina") as int

    
    float playerVelocity = Caster.GetAnimationVariableFloat("Speed")
    
    if Kick_MCM_Settings.slowTimeOnKick
      TimeSlowSpell.Cast(Caster, None)
    endif
    hostileSpell.Cast(Caster, target)

    float DRMultiplier = CalculateDRMultiplier(target)


    ; should armor also reduce the knockback effect? That way jumping and running kicks will be more important to use agains heavily armored enemies

    float kickForce = Kick_MCM_Settings.baseKickForce * CalculateDRMultiplier(target)

    
    kickForce = kickForce + (playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)

    ; damage the enemy
    float damage = (Kick_MCM_Settings.baseKickDamage + (playerStamina  * Kick_MCM_Settings.additionalDamagePerStamina)) * DRMultiplier    

    damage = damage + (playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)

    float staminaDamage = ((Kick_MCM_Settings.baseKickStaminaDamage + (playerStamina  * Kick_MCM_Settings.additionalStaminaDamagePerStamina)) * DRMultiplier) * 1

    staminaDamage = staminaDamage + (playerVelocity * Kick_MCM_Settings.additionalVelocityForce / 1000.0)

    ; check if player is jumping, if so it's a jumping kick
    if Caster.GetAnimationVariableBool("bInJumpState")
      kickForce = kickForce + (Kick_MCM_Settings.baseKickForce * (Kick_MCM_Settings.jumpKickBonus / 100.0))
      ; Debug.Notification("Jump kick bonus!")
    endif

    float relativeDirection = math.abs(target.GetHeadingAngle(Caster))

    ; Debug.Notification("Relative direction: " + relativeDirection)

    ; this checks if the player is behind or to the side and if so applies a bonus to the kick force
    If relativeDirection >= 140
      kickForce = kickForce + (Kick_MCM_Settings.baseKickForce * (Kick_MCM_Settings.backKickBonus / 100.0))
      ; Debug.Notification("Kicking from the back bonus!")
    elseif relativeDirection >= 70
      kickForce = kickForce + (Kick_MCM_Settings.baseKickForce * ((Kick_MCM_Settings.backKickBonus / 100.0) * 0.5))
      ; Debug.Notification("Kicking from the side bonus!")
    endif

    HitSoundHeavy.play(Caster)
    
    ; ok since we can't get velocity now, we'll just use the player's current speed for now


    kickForce = kickForce * DRMultiplier
    ; Debug.Notification("Player Velocity: " + playerVelocity)
    ; Debug.Notification("Final Kick force: " + kickForce)

    ; send that bitch flying
    Caster.PushActorAway(target, kickForce)
    
    Utility.wait(0.1)    

    target.ApplyHavokImpulse(0.0, 0.0, 0.1, kickForce)

    target.DamageActorValue("Stamina", staminaDamage)

    target.DamageActorValue("Health", damage)

EndFunction


Float Function CalculateDRMultiplier(Actor target)
    float result = 1.0
    if !target
        return result
    endif
    float dr = 0.0
    Float ar = target.GetActorValue("DamageResist")
    dr = ar * Kick_MCM_Settings.armorRaitingMultiplier
    if dr > 80.0
        dr = 80.0
    endif
    result = (1.0 - dr / 100.0)
    return result
EndFunction