Scriptname sot_genesisdist extends ObjectReference  



Event OnUpdate() 

Debug.notification("Player is " + Game.GetPlayer().GetDistance(skulltomeasure) + " units away from skull)")



EndEvent

 
Event OnActivate(ObjectReference akActionRef)
  Debug.Trace("Activated by " + akActionRef)
  
  Debug.notification("Player is " + Game.GetPlayer().GetDistance(skulltomeasure) + " units away from skull)")
  	RegisterForUpdate(3.0)
EndEvent
 
 


Event OnItemRemoved(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akDestContainer)


    Debug.Trace("I dropped " + aiItemCount + "x " + akBaseItem + " into the world")

	RegisterForUpdate(3.0)
	
	




EndEvent


Event OnItemAdded(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akSourceContainer)

RegisterForUpdate(3.0)

endEvent


ObjectReference Property skulltomeasure  Auto  
