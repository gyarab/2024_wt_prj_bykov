from django.db import models
from django.core.exceptions import ValidationError
from django.utils.translation import gettext_lazy as gt

# char field limited and faster
# text field slower

# server side validation
# https://docs.djangoproject.com/en/5.1/ref/validators/
def validateUsername(value):
    if value is "fuj":
        raise ValidationError(gt("fuj se nesmi!"), params={"value": value})
    return

class SearchUser(models.Model):
    username = models.CharField(null=False, blank=False, max_length=64, validators=[validateUsername])
    userIconId = models.BigIntegerField(null=False, blank=False, default=-1)
    userDescription = models.TextField(null=False, blank=True, default="I am a free NPWS user!")
    # attrib ids from middle table
    friends = models.ForeignKey("self", null=True, on_delete=models.SET_NULL) #attrib to self
    recentSearches = models.CharField(max_length=100)
    # keyword from middle table

    def __str__(self):
        return f"{self.username} {self.userIconId} ({self.userDescription}) recently searched {self.recentSearches}"

class KeywordList(models.Model):
    keyword = models.CharField(null=False, blank=False, max_length=30)
    # user in middle table

    def __str__(self):
        return f"{self.keyword} keyword"

class AllowedAttributeList(models.Model):
   name = models.CharField(null=False, blank=False, max_length=64)
   value = models.CharField(null=False, blank=False, max_length=64)

   def __str__(self):
        return f"{len(self)} allowed attributes of {self.name}"

class SearchUserAttributes(models.Model):
    name = models.CharField(null=False, blank=False, max_length=64)
    value = models.CharField(null=False, blank=False, max_length=64)
    username = models.CharField(null=False, blank=False, max_length=64)

    def __str__(self):
        return f"{len(self)} allowed attributes of {self.name}" 

# m...n middle table between user and keyword list
class SearchUserKeywordListMiddleTable(models.Model):
    user = models.ForeignKey(SearchUser, on_delete=models.CASCADE)
    keywordList = models.ForeignKey(KeywordList, on_delete=models.CASCADE)
    
# m...n middle table between user and attribs
class SearchUserAttributesMiddleTable(models.Model):
    user = models.ForeignKey(SearchUser, on_delete=models.CASCADE)
    attributes = models.ForeignKey(SearchUserAttributes, on_delete=models.CASCADE)
