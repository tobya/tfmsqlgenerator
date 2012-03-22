<?php
/*************************************************************
Copyright © 2012 Toby Allen (http://github.com/tobya)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sub-license, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice, and every other copyright notice found in this software, and all the attributions in every file, and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
****************************************************************/
/** sql_query_generator V 0.5
* 
* This class is designed to take a standard set of inputs from a post or get form and 
 * turn it into a sql select query on a database.  
 * Basic Fields Required
 * 
 * One of the two must exist.
 * 		table - Table name for query.
 * 		join - Join SQL - overrides table if exists.
 * 		if both exist join is used.
 * 
 * The following Can Exist.
 * 		orderby - Order by Clause - without ORDER BY
 * 		groupby - Group By Clause - without Group By
 * 		usehaving - 'true' or 'false' (default) will use Having instead of WHERE must be used with Group BY
 * 
 * At least one field value - Operator pair must exist.				
 * 1Input should be in the following format.
 * 2Where FieldName is the name of the field.
 * 3fldFieldName_Value = Value to be searched for.
 * 4fldFieldName_Operator = Operator on search 
 * 		1Possible Values for Operator
 * 			1 blank - No Value selected (wont be included in statement).
 * 			2 eq - Equals
 * 			3 gt - greater than
 * 			lt - lessthan
 * 			lk - Like %val%
 * 			lkb - like begining val%
 * 			lke - like end %val
 * 			not - not equal.
 *selectfields = All fields to be selected in sql. Defaults to * if not present.
 *aliasFieldName = Sql for an As Field.
 * - this is primarily to alias calculated fields, so when generated the any time the field
 *   specified by FieldName above will be replaced by the sql value of the field.  eg. You
 *   have Sum(Price) as Total create hidden field name='aliasTotal' value='Sum(Price)' now 
 *   can use the field total in the form and anytime the class find it it will be replaced 
 *   with Sum(Price) in the sql.
 * 
 *---------------------------
 * Prefix.
 * - In order to generate more than one sql query from a particular form, it is possible to 
 *   specify a prefix when generating the sql.  The code will then only work with fields that
 *   begin with this prefix, so prefixfldFieldName is used instead of fldFieldName etc.
 *   Prefix can be used with all fields specified above.  This was developed because mysql
 *   (which I used this for) doesnt provide views.
 *
 * 
 * To use:
 * 		Ensure all required fields exist in form and submit.
 * 		create an instance of sql_query_generator
 * 		Call getSQL([$Prefix]) where prefix is a prefix you place before all fields for a specific query.  
 * 			Allows multiple queries to be generated from one form. Prefix is "" empty string by default.
 * 		Use the SQL.
 *
 * Other Callable Functions:
 * 	SetFormFieldIdentifier - by default all fields must have 'fld' at the begining to be 
 * 							 recognized, you may change this here.
 *  setAliasFieldIdentifier - by default all alias fields must have 'alias' at the begining to 
 * 							  be recognised. You may change this.	
 * Variables that can be accessed
 * $BuildEnglishQuery (Boolean) if true build a rough english representation of query. Default false.
 * $EnglishQueryDescription (String) if above is true after calling getSQL() this variable contains the
 * 									 english reprentation of query.
 * Copyright Toby Allen 2003
 **/
 
 
 /*
 * TODO
 * Change % in like statement to changable const or variable.
 */
 
class sql_query_generator{

	var $WhereStarted;  //Boolean indicating if 1 or more where clauses exist.
	var $table;
	var $join; //sql string indicating join clause if required.
	var $VARS; //GET or POST Variables.
	var $Arr_FieldValues;
	var $Arr_FieldOpers;
	var $SelectFields;
	var $FromClause;
	var $OrderBy;
	var $GroupBy;
	var $UseHaving;
	var $CurrentPrefix;			//Prefix to all fields used for call to  getsql;
	var $FormFieldIdentifier;   //will be 'fld' or another specified word.
	var $CalcFieldAliases;		//Used to store sql for Calculated field eg. Sum(Price) as Total. Total will be the key and Sum(Price) the value.
	var $AliasFieldIdentifier;  //will be 'alias' or another specified word.
	var $BuildEnglishQuery;
	var $EnglishQueryDescription;
	var $DateFields;  //Array of fields that should be treated as dates.
	/**
     * Constructor
     * @access protected
     */
	function sql_query_generator()
	{
		$this->WhereStarted = false;	
		$this->_getVARS();
		$this->FormFieldIdentifier 	= 'fld';
		$this->AliasFieldIdentifier = 'alias';		
		//$this->Setup();
	}
	/**
	 * Setup is generally called internally.  When a request is made to GetSQL, Setup is called
	 * to setup the correct prefix and ensure all variables are reset for a new pass. 
	 * @access protected
	 * @param $UsePrefix - Default "" empty string - Prefix to form fields to be used.
	 * @param $DoEnglishAlso - Default false - if true EnglishQueryDescription will have a very
	 * basic "English" representation of query.
	 **/	 
	function Setup($UsePrefix = "",$DoEnglishAlso = false)
	{
		//Reset Fields
		$this->WhereStarted = 		false;
		$this->Arr_FieldOpers = 	array();
		$this->Arr_FieldValues = 	array();
		$this->CurrentPrefix = 		$UsePrefix;
		$this->CalcFieldAliases = 	array();
		$this->SelectFields = 		"";
		$this->UseHaving = 			false;
		$this->DateFields = array();		
		
		
		$this->BuildEnglishQuery = $DoEnglishAlso;
		if ($this->BuildEnglishQuery) 
		{
			$this->EnglishQueryDescription = " Searching for results that match the following criteria. "    ;
		}
		
		$this->table =  $this->_GetValue($this->CurrentPrefix ."table" );
		$this->join = $this->_GetValue($this->CurrentPrefix .'join' );
		$this->getCalcFieldAliases();
		
		//Get table or join sql.	
		if (!$this->join === false)  
		{
			$this->FromClause = " FROM $this->join ";		    
		}
		else if ((!$this->table === false)) 
		{
			$this->FromClause = " FROM $this->table ";         
		}
		else
			$this->doError('You must provide a table name or join value.',true );
		
		//get order by clause
		$this->OrderBy = $this->_GetValue($this->CurrentPrefix .'orderby','' );
		if ($this->OrderBy <> "") 
		{
			$this->OrderBy = stripslashes(" Order by " . $this->getalias($this->OrderBy)  )   ;

		}	
		
		//get Group By Clause
		$this->GroupBy = $this->_GetValue($this->CurrentPrefix .'groupby','' );
		if ($this->GroupBy <> "") 
		{
			$this->GroupBy = stripslashes(" Group by " . $this->GroupBy  )   ;
		}			
		
		//Fields to return.
		$this->SelectFields = stripslashes( $this->_GetValue($this->CurrentPrefix .'selectfields','*' ));	
		
		
		$this->DateFields = $this->listDateFields();
		$this->setUseHaving($this->_GetValue($this->CurrentPrefix .'usehaving',false ));
	}
	
	/**
	 * getSQL is the main functions.  On a call to getSQL with a specific prefix the sql code 
	 * generated will be returned.
	 * @access protected
	 * @param $Prefix - Default "" empty string - Prefix to form fields to be used.
	 * @param $DoEnglishAlso - Default false - if true EnglishQueryDescription will have a very
	 * basic "English" representation of query.
	 * @return String - SQL.
	 **/		
	function getSQL($Prefix = "",$DoEnglishAlso = false)
	{
		$SQL = "";
		
		//Reset values and specify prefix to setup.
		$this->Setup($Prefix,$DoEnglishAlso);
		
		//Get fields and values.
		$this->_BuildFieldArrays();
		

		
		//Build where clause out of fields and values.
		foreach($this->Arr_FieldValues as $Key => $Value )
		{
			if ($Value <> "") {
			    $SQL .= $this->_addWhereClause($Key,$Value  , $this->Arr_FieldOpers[$Key] );
			}
				
		}
		
		

				
		if ($this->WhereStarted) 
		{
			//This is a simplistic way of creating a having clause it may very possibly return
			//invalid sql.
			if ($this->UseHaving) 
			{
				$SQL = "Select $this->SelectFields  $this->FromClause  $this->GroupBy HAVING $SQL";    
			}
			else
			{
				$SQL = "Select $this->SelectFields  $this->FromClause Where $SQL $this->OrderBy";    
			}
		}
		//No SQL will be returned if there is no where clause.
		
		return $SQL;
	}
	/**
	 * sql_query_generator :: _addWhereClause()
	 * Takes a fieldname, the value to search for and the operator to use,
	 * returns section of where clause generated.
	 * @access protected
	 * @param $Field
	 * @param $Value
	 * @param $Operator
	 * @return String - Section of Where Clause Generated
	 **/
	function _addWhereClause($Field , $Value, $Operator)
	{
		
		$Whr = "";

		$ANDString = "";
		if ($this->WhereStarted) 
		{
			$ANDString = " and ";    
		}
		
		$Field = $this->getAlias($Field);

		//echo "AField - " . $Field . "<P>";
		if (is_array($this->DateFields)) 
		{
		   	if (array_key_exists("$Field",$this->DateFields)) 
			{
			
			list($day,$month,$year) = explode("/",$Value);
		 	$Value = "$year-$month-$day";      
			}	 
		}


		
			
		//Find out what operator is requested for field / value and insert relevant sql.
		switch($Operator){
			case "eq": 
				$Whr = " $ANDString $Field = '$Value'";
				$this->WhereStarted = true;
				if ($this->BuildEnglishQuery) 
				{
					$this->EnglishQueryDescription .= " $ANDString \"$Field\"= '$Value'" ;   
				}				
				break;
			case "gt": 
				$Whr = " $ANDString $Field > '$Value'";
				$this->WhereStarted = true;
				if ($this->BuildEnglishQuery) 
				{
					$this->EnglishQueryDescription .= " $ANDString \"$Field\" is greater than $Value" ;   
				}								
				break;
			case "lt": 
				$Whr = " $ANDString $Field < '$Value'";	
				$this->WhereStarted = true;			
				if ($this->BuildEnglishQuery) 
				{
					$this->EnglishQueryDescription .= " $ANDString \"$Field\" is less than '$Value'" ;   
				}								
				break;				
			case "lk": 
				$Whr = " $ANDString $Field like '%$Value%'";
				$this->WhereStarted = true;
				if ($this->BuildEnglishQuery) 
				{
					$this->EnglishQueryDescription .= " $ANDString \"$Field\"  contains the value '$Value'" ;   
				}								
				break;				
			case "lkb": 
				$Whr = " $ANDString $Field like '$Value%'";
				if ($this->BuildEnglishQuery) 
				{
					$this->EnglishQueryDescription .= " $ANDString \"$Field\" begins with the value '$Value'"    ;
				}								
				$this->WhereStarted = true;
				break;					
			case "lke": 
				$Whr = " $ANDString $Field like '%$Value'";
				$this->WhereStarted = true;
				if ($this->BuildEnglishQuery) 
				{
					$this->EnglishQueryDescription .= " $ANDString \"$Field\" ends with the value '$Value'"  ;  
				}								
				break;		
			case "not": 
				$Whr = " $ANDString NOT ($Field  = '$Value')";
				$this->WhereStarted = true;
				if ($this->BuildEnglishQuery) 
				{
					$this->EnglishQueryDescription .= " $ANDString \"$Field\" is not equal to '$Value'"  ;  
				}								
				break;														
		} // switch
				
		return $Whr;
	}
	/**
	 * sql_query_generator :: getAlias()
	 * 
	 * @access 
	 * @param FieldName - String 
	 * @return String - Alias for Field or just field if no alias.
	 **/	
	function getAlias($FieldName)
	{
		
		if (array_key_exists($FieldName,$this->CalcFieldAliases)) 
		{
				
		 	$FieldName = $this->CalcFieldAliases[$FieldName];      
			
		}
	
		return $FieldName;	
	}
	
	function _BuildFieldArrays()
	{
		$RemovableStringLength = strlen($this->CurrentPrefix)+ strlen($this->FormFieldIdentifier);
		foreach($this->VARS as $Key => $Value )
		{
			$pos = strpos($Key,$this->CurrentPrefix .$this->FormFieldIdentifier);
			if (($pos !== false)  and $pos == 0)
			{
				
				   
			
				$ValueArr = explode('_',$Key);
				$tmpKey = substr($ValueArr[0],$RemovableStringLength); 
				
				
				if ($ValueArr[1] == 'Value') 
				{

				 	$this->Arr_FieldValues[$tmpKey] = $Value;   
				}
				else
					$this->Arr_FieldOpers[$tmpKey] = $Value;  
			}	 				
		}
		
	}
	
	function getCalcFieldAliases()
	{
		$RemovableStringLength = strlen($this->CurrentPrefix)+ strlen($this->AliasFieldIdentifier);
		foreach($this->VARS as $Key => $Value )
		{
			$pos = strpos($Key,$this->CurrentPrefix .$this->AliasFieldIdentifier);
			if (($pos !== false)  and $pos == 0)
			{	
										 
				$tmpKey = substr($Key,$RemovableStringLength); 
				
				$this->CalcFieldAliases[$tmpKey] = $Value;
				  
			}	 				
		}	
	}
	
	function _GetValue($ValueName, $Default = false)
	{
		if (isset($this->VARS[$ValueName])) {
			return $this->VARS[$ValueName];		    	
		}
		else
			return  $Default;		
	
	}
	
	function setFormFieldIdentifier($Value)
	{
		$this->FormFieldIdentifier = $Value;
	}

	function setAliasFieldIdentifier($Value)
	{
		$this->AliasFieldIdentifier = $Value;
	}
	
	
	function setUseHaving($Value)
	{
		if (is_bool($Value)) 
		{
			
			$this->UseHaving = $Value;    
		}
		else if ($Value === 'true') 
		{
			
			$this->UseHaving = true;	         
	     }
		else
		{
						
			$this->UseHaving = false;
		}
	
	}
	
	function listDateFields()
	{
		$RemovableStringLength = strlen($this->CurrentPrefix)+ strlen($this->FormFieldIdentifier);
		
		if (isset($this->VARS[$this->CurrentPrefix . "datefields"])) 
		{

			$tmparray = explode(",",$this->VARS[$this->CurrentPrefix . "datefields"]);   
			foreach($tmparray as  $Value )
			{
				$tmpKey = substr($Value,$RemovableStringLength);
				$tmparray[$tmpKey] = '0';	
			}
			return $tmparray;
		}
					
	}
	
	
	function _getVARS()
	{
		Global $HTTP_POST_VARS;
		GLOBAL $HTTP_GET_VARS;
		Global $HTTP_SERVER_VARS;


		$METHOD = $HTTP_SERVER_VARS['REQUEST_METHOD'];

		//Get reference to request array needed.
		if ($METHOD == 'GET')
		{
			$this->VARS =& $HTTP_GET_VARS;
			
		}
		else if ($METHOD == 'POST')
		{
			$this->VARS =& $HTTP_POST_VARS;
			
		}
		else
			doerror('No Post or Get Variables');

	}

	function doError($MSG,$ShouldExit = false)
	{
		echo $MSG;
		if ($ShouldExit) {
		    exit;
		}
	}
}

?>