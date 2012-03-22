<html>
<Body>
<H1>Very simple results page.</H1>
<?php



	require("class.tfm.sqlquery.inc");

	$sqlgen = new sql_query_generator;
		
	echo	$sqlgen->getSQL();
	echo "<P>";
	//CHECK CORRECT FORM HAS BEEN SUBMITTED FOR USING PREFIX TOTAL.
	if (isset($_POST["TotalaliasTotalPurchased"])) 
	{
	    echo $sqlgen->getSQL("Total");
	}
	
?><a href=index.html>Back to search</a></Body></html>