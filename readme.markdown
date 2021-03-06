  This class is designed to take a standard set of inputs from a post or get form and turn it into a sql select query on a database.  

###Basic Fields Required###
   
One of the two must exist.

-  table - Table name for query.
-  join - Join SQL - overrides table if exists.

if both exist join is used.
   

The following Can Exist.

- orderby - Order by Clause - without ORDER BY
- groupby - Group By Clause - without Group By
- usehaving - 'true' or 'false' (default) will use Having instead of WHERE must be used with Group BY
   
At least one field value - Operator pair must exist.				

   1. Input should be in the following format.
   2. Where FieldName is the name of the field.
   3. fldFieldName_Value = Value to be searched for.
   4. fldFieldName_Operator = Operator on search 
   		
###Possible Values for Operator###

- blank - No Value selected (wont be included in statement).
- eq - Equals
- gt - greater than
- lt - lessthan
- lk - Like %val%
- lkb - like begining val%
- lke - like end %val
- not - not equal.
  
**selectfields** 

All fields to be selected in sql. Defaults to   if not present.

**aliasFieldName** (Sql for an As Field.)

This is primarily to alias calculated fields, so when generated the  any time the field specified by FieldName above will be replaced by the  sql value of the field.  eg. You have Sum(Price) as Total create hidden  field name='aliasTotal' value='Sum(Price)' now can use the field total in the form and anytime the class find it it will be replaced with Sum(Price) in the sql.
   
---------------------------

**Prefix.**

In order to generate more than one sql query from a particular form, it is possible to specify a prefix when generating the sql.  The code will then only work with fields that begin with this prefix, so prefixfldFieldName is used instead of fldFieldName etc. Prefix can be used with all fields specified above.  This was developed because mysql
(which I used this for) doesnt provide views.
  
   
To use:

- Ensure all required fields exist in form and submit.
- create an instance of sql_query_generator
- Call getSQL([$Prefix]) where prefix is a prefix you place before all fields for a specific query.  
- Allows multiple queries to be generated from one form. Prefix is "" empty string by default.
- Use the SQL.
  
  
###Other Callable Functions:###

- SetFormFieldIdentifier - by default all fields must have 'fld' at the begining to be recognized, you may change this here.
- setAliasFieldIdentifier - by default all alias fields must have 'alias' at the begining to  be recognised. You may change this.	

###Variables that can be accessed###

- $BuildEnglishQuery (Boolean) if true build a rough english representation of query. Default false.
- $EnglishQueryDescription (String) if above is true after calling getSQL() this variable contains the
   									 english reprentation of query.