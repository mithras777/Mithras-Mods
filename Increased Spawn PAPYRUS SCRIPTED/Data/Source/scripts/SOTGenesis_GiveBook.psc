Scriptname SOTGenesis_GiveBook extends Quest  

Book Property GenesisOptions  Auto  

GlobalVariable Property BookAdded  Auto 


Event OnPlayerLoadGame ()


;if BookAdded.getvalueint() == 0
;BookAdded.setvalueint(5)
;game.getplayer().additem(GenesisOptions, 1)

;endif

endevent

;Event OnInit()
;
;if (Game.GetPlayer().GetItemCount(GenesisOptions) == 0) && BookAdded.getvalueint() == 0
 ;BookAdded.setvalueint(5)
 ;Debug.Messagebox("Genesis Mod Book Added to Inventory!")
  ;Game.GetPlayer().AddItem(GenesisOptions)
;endIf	

;
;endevent