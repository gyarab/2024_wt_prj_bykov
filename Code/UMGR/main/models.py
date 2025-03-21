from django.db import models

# Create your models here.

class SearchUserAttributes(models.Model):
    pass

class SearchUser(models.Model):
    username = models.CharField(null=False, blank=False, max_length=64, primary_key=True)
    userIconId = models.BigIntegerField(null=False, blank=False, default=-1)
    userDescription = models.TextField(max_length=140, null=False, blank=True, default="I am a free NPWS user!")
    # TODO arrays
    attribIds = models.ForeignKey('Attributes', null=True, on_delete=models.SET_NULL)
    friends = models.JSONField()
    recentSearches = models.CharField(max_length=100)
    keywordIds = models.JSONField()

    def __str__(self):
        return f"{self.username} {self.userIconId} ({self.userDescription}) recently searched {self.recentSearches}"
    
class KeywordList(models.Model):
    keyword = models.CharField(null=False, blank=False, max_length=30, primary_key=True)
    users = "" # TODO

    def __str__(self):
        return f"{self.keyword} searched by {len(users)} users"

class AllowedAttributeList(models.Model):
   values = 

   def __str__(self):
        return f"{len(values)} allowed attributes of "

class SearchUserAttribute(models.Model):
    name = models.CharField(null=False, blank=False, max_length=64, primary_key=True)
    users = 
    values = 